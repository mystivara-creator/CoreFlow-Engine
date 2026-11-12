#include "coreflow/mount_topology.hpp"

#include <algorithm>
#include <charconv>
#include <cerrno>
#include <cmath>
#include <dirent.h>
#include <filesystem>
#include <fcntl.h>
#include <fstream>
#include <limits>
#include <set>
#include <sstream>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>

namespace coreflow {
namespace fs = std::filesystem;
namespace {

std::string decode_mount_field(std::string_view value) {
    std::string out;
    out.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '\\' && i + 3U < value.size()) {
            const auto code = value.substr(i + 1U, 3U);
            if (code == "040") { out.push_back(' '); i += 3U; continue; }
            if (code == "011") { out.push_back('\t'); i += 3U; continue; }
            if (code == "012") { out.push_back('\n'); i += 3U; continue; }
            if (code == "134") { out.push_back('\\'); i += 3U; continue; }
        }
        out.push_back(value[i]);
    }
    return out;
}

bool parse_u64(std::string_view s, uint64_t& value) noexcept {
    if (s.empty()) return false;
    const auto parsed = std::from_chars(s.data(), s.data() + s.size(), value);
    return parsed.ec == std::errc{} && parsed.ptr == s.data() + s.size();
}

std::vector<std::string> split(std::string_view value, char delimiter) {
    std::vector<std::string> result;
    std::size_t pos = 0;
    while (pos <= value.size()) {
        const auto end = value.find(delimiter, pos);
        const auto token = value.substr(pos, end == std::string_view::npos ? value.size() - pos : end - pos);
        if (!token.empty()) result.emplace_back(token);
        if (end == std::string_view::npos) break;
        pos = end + 1U;
    }
    return result;
}

bool has_token(const std::vector<std::string>& values, std::string_view token) {
    return std::find(values.begin(), values.end(), token) != values.end();
}

bool safe_absolute_path(std::string_view path) noexcept {
    if (path.empty() || path.front() != '/' || path.find('\\') != std::string_view::npos) return false;
    std::size_t pos = 0;
    while (pos < path.size()) {
        while (pos < path.size() && path[pos] == '/') ++pos;
        const auto end = path.find('/', pos);
        const auto part = path.substr(pos, end == std::string_view::npos ? path.size() - pos : end - pos);
        if (part == ".." || part == ".") return false;
        if (end == std::string_view::npos) break;
        pos = end + 1U;
    }
    return true;
}

bool path_is_within(std::string_view path, std::string_view root) noexcept {
    if (root == "/") return !path.empty() && path.front() == '/';
    return path == root || (path.size() > root.size() && path.substr(0, root.size()) == root &&
                            path[root.size()] == '/');
}

std::string relative_to_root(std::string_view path, std::string_view root) {
    if (root == "/") return path.size() > 1U ? std::string(path.substr(1U)) : std::string{};
    if (path == root) return {};
    return std::string(path.substr(root.size() + 1U));
}

bool exists_regular_no_symlink(const fs::path& path) noexcept {
    struct stat st{};
    return ::lstat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode);
}

std::optional<std::string> read_small(const fs::path& path, std::size_t limit = 16384U) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return std::nullopt;
    std::string value;
    value.reserve(std::min<std::size_t>(limit, 256U));
    char buffer[512];
    while (input) {
        input.read(buffer, sizeof(buffer));
        const auto count = static_cast<std::size_t>(input.gcount());
        if (value.size() + count > limit) return std::nullopt;
        value.append(buffer, count);
    }
    if (!input.eof()) return std::nullopt;
    return value;
}

std::optional<CgroupMembershipInterface> detect_membership(const fs::path& dir) {
    // Prefer the thread ABI when both interfaces exist: migration targets TIDs.
    if (exists_regular_no_symlink(dir / "cgroup.threads")) return CgroupMembershipInterface::Threads;
    if (exists_regular_no_symlink(dir / "tasks")) return CgroupMembershipInterface::Tasks;
    if (exists_regular_no_symlink(dir / "cgroup.procs")) return CgroupMembershipInterface::Processes;
    return std::nullopt;
}

bool relevant_mount(const CgroupMount& mount) {
    if (mount.filesystem == CgroupFilesystem::V2) return true;
    return has_token(mount.controllers, "cpu") || has_token(mount.controllers, "cpuctl") ||
           has_token(mount.controllers, "cpuset");
}

bool cpu_controller_mount(const CgroupMount& mount);

std::string membership_join(const CgroupMount& mount, std::string_view relative) {
    if (relative.empty()) return mount.mount_point;
    return (fs::path(mount.mount_point) / fs::path(relative)).lexically_normal().string();
}

} // namespace

std::vector<CgroupMount> MountTopologyResolver::discover_cgroup_mounts() const {
    std::vector<CgroupMount> mounts;
    const auto content = read_small(fs::path(proc_root_) / "self/mountinfo", 1024U * 1024U);
    if (!content) return mounts;
    std::istringstream lines(*content);
    std::string line;
    while (std::getline(lines, line)) {
        const auto separator = line.find(" - ");
        if (separator == std::string::npos) continue;
        std::istringstream before(line.substr(0, separator));
        std::string id_s, parent_s, device, root_s, mount_s;
        if (!(before >> id_s >> parent_s >> device >> root_s >> mount_s)) continue;
        uint64_t id = 0, parent = 0;
        if (!parse_u64(id_s, id) || !parse_u64(parent_s, parent)) continue;

        std::istringstream after(line.substr(separator + 3U));
        std::string fs_type, source, super_options;
        if (!(after >> fs_type >> source >> super_options)) continue;
        if (fs_type != "cgroup" && fs_type != "cgroup2") continue;

        CgroupMount mount{};
        mount.mount_id = id;
        mount.parent_id = parent;
        mount.filesystem = fs_type == "cgroup2" ? CgroupFilesystem::V2 : CgroupFilesystem::V1;
        mount.root = decode_mount_field(root_s);
        mount.mount_point = decode_mount_field(mount_s);
        mount.controllers = split(super_options, ',');
        const auto option_fields = split(line.substr(0, separator), ' ');
        if (option_fields.size() > 5U) {
            const auto options = split(option_fields[5], ',');
            mount.read_only = has_token(options, "ro");
        }
        if (!safe_absolute_path(mount.root) || !safe_absolute_path(mount.mount_point) || !relevant_mount(mount)) continue;
        mounts.push_back(std::move(mount));
    }
    std::sort(mounts.begin(), mounts.end(), [](const CgroupMount& a, const CgroupMount& b) {
        if (a.mount_point != b.mount_point) return a.mount_point < b.mount_point;
        return a.mount_id < b.mount_id;
    });
    return mounts;
}

std::vector<CgroupGroupCapability> MountTopologyResolver::discover_cgroup_groups(
    std::size_t max_directories, unsigned max_depth, bool cpu_only) const {
    std::vector<CgroupGroupCapability> groups;
    if (max_directories == 0U) return groups;
    auto mounts = discover_cgroup_mounts();
    if (cpu_only) {
        mounts.erase(std::remove_if(mounts.begin(), mounts.end(), [](const CgroupMount& mount) {
            return !cpu_controller_mount(mount);
        }), mounts.end());
    }
    std::size_t visited = 0;
    for (const auto& mount : mounts) {
        struct Pending { fs::path path; unsigned depth; };
        std::vector<Pending> pending{{fs::path(mount.mount_point), 0U}};
        while (!pending.empty() && visited < max_directories) {
            auto current = std::move(pending.back());
            pending.pop_back();
            ++visited;
            std::error_code ec;
            const auto status = fs::symlink_status(current.path, ec);
            if (ec || fs::is_symlink(status) || !fs::is_directory(status)) continue;

            CgroupGroupCapability group{};
            group.mount = mount;
            group.absolute_path = current.path.lexically_normal().string();
            const auto relative = fs::path(group.absolute_path).lexically_relative(mount.mount_point);
            if (relative.empty() && group.absolute_path != mount.mount_point) continue;
            group.relative_path = relative == "." ? std::string{} : relative.string();
            if (group.relative_path.find("..") != std::string::npos) continue;

            std::error_code control_ec;
            for (fs::directory_iterator it(current.path, fs::directory_options::skip_permission_denied, control_ec), end;
                 !control_ec && it != end; it.increment(control_ec)) {
                if (!exists_regular_no_symlink(it->path())) continue;
                const auto name = it->path().filename().string();
                const auto suffix_match = [&name](std::string_view suffix) {
                    return name == suffix || (name.size() > suffix.size() &&
                        name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0);
                };
                if (suffix_match("uclamp.min")) {
                    group.has_uclamp_min = true;
                    group.uclamp_min_path = it->path().string();
                } else if (suffix_match("uclamp.max")) {
                    group.has_uclamp_max = true;
                    group.uclamp_max_path = it->path().string();
                } else if (suffix_match("uclamp.latency_sensitive")) {
                    group.has_uclamp_latency_sensitive = true;
                    group.uclamp_latency_sensitive_path = it->path().string();
                } else if (name == "io.weight") {
                    group.has_io_weight = true;
                    group.io_weight_path = it->path().string();
                }
            }
            if (const auto membership = detect_membership(current.path)) group.membership = *membership;
            group.has_process_membership = group.membership != CgroupMembershipInterface::None;
            group.has_process_list = exists_regular_no_symlink(current.path / "cgroup.procs");
            if ((group.has_uclamp_min && group.has_uclamp_max) || group.has_process_membership || group.has_io_weight)
                groups.push_back(std::move(group));

            if (current.depth >= max_depth) continue;
            std::error_code iter_ec;
            for (fs::directory_iterator it(current.path, fs::directory_options::skip_permission_denied, iter_ec), end;
                 !iter_ec && it != end && visited + pending.size() < max_directories; it.increment(iter_ec)) {
                std::error_code child_ec;
                const auto child_status = it->symlink_status(child_ec);
                if (child_ec || fs::is_symlink(child_status) || !fs::is_directory(child_status)) continue;
                pending.push_back({it->path(), current.depth + 1U});
            }
        }
        if (visited >= max_directories) break;
    }
    return groups;
}

std::vector<ProcessCgroupMembership> MountTopologyResolver::process_memberships(
    int pid, std::size_t max_bytes) const {
    std::vector<ProcessCgroupMembership> result;
    if (pid <= 0) return result;
    const auto content = read_small(fs::path(proc_root_) / std::to_string(pid) / "cgroup", max_bytes);
    if (!content) return result;
    const auto mounts = discover_cgroup_mounts();
    std::istringstream lines(*content);
    std::string line;
    while (std::getline(lines, line)) {
        const auto first = line.find(':');
        const auto second = first == std::string::npos ? std::string::npos : line.find(':', first + 1U);
        if (first == std::string::npos || second == std::string::npos) continue;
        std::string_view hierarchy(line.data(), first);
        std::string_view controllers(line.data() + first + 1U, second - first - 1U);
        std::string cgroup_path = line.substr(second + 1U);
        if (!safe_absolute_path(cgroup_path)) continue;
        uint64_t hierarchy_id = 0;
        (void)parse_u64(hierarchy, hierarchy_id);
        const auto active = split(controllers, ',');
        for (const auto& mount : mounts) {
            bool matches = false;
            if (mount.filesystem == CgroupFilesystem::V2) matches = controllers.empty() && hierarchy_id == 0U;
            else {
                matches = std::any_of(active.begin(), active.end(), [&](const std::string& token) {
                    return has_token(mount.controllers, token);
                });
            }
            if (!matches || !path_is_within(cgroup_path, mount.root)) continue;
            auto relative = relative_to_root(cgroup_path, mount.root);
            ProcessCgroupMembership item{};
            item.mount = mount;
            item.cgroup_path = cgroup_path;
            item.relative_path = std::move(relative);
            item.absolute_path = membership_join(mount, item.relative_path);
            result.push_back(std::move(item));
        }
    }
    return result;
}

std::optional<ProcessCgroupMembership> MountTopologyResolver::resolve_process_membership(
    int pid, const CgroupMount& mount) const {
    if (pid <= 0) return std::nullopt;
    const auto content = read_small(fs::path(proc_root_) / std::to_string(pid) / "cgroup", 16384U);
    if (!content) return std::nullopt;
    std::istringstream lines(*content);
    std::string line;
    while (std::getline(lines, line)) {
        const auto first = line.find(':');
        const auto second = first == std::string::npos ? std::string::npos : line.find(':', first + 1U);
        if (first == std::string::npos || second == std::string::npos) continue;
        const std::string_view hierarchy(line.data(), first);
        const std::string_view controllers(line.data() + first + 1U, second - first - 1U);
        uint64_t hierarchy_id = 0U;
        const bool parsed_hierarchy = parse_u64(hierarchy, hierarchy_id);
        const auto active = split(controllers, ',');
        const bool matches = mount.filesystem == CgroupFilesystem::V2
            ? parsed_hierarchy && hierarchy_id == 0U && controllers.empty()
            : std::any_of(active.begin(), active.end(), [&](const std::string& token) {
                  return has_token(mount.controllers, token);
              });
        if (!matches) continue;
        std::string cgroup_path = line.substr(second + 1U);
        if (!safe_absolute_path(cgroup_path) || !path_is_within(cgroup_path, mount.root))
            return std::nullopt;
        ProcessCgroupMembership item{};
        item.mount = mount;
        item.cgroup_path = cgroup_path;
        item.relative_path = relative_to_root(cgroup_path, mount.root);
        item.absolute_path = membership_join(mount, item.relative_path);
        return item;
    }
    return std::nullopt;
}

std::optional<CgroupGroupCapability> MountTopologyResolver::resolve_foreground_group() const {
    struct Candidate { CgroupGroupCapability group; int oom_score{std::numeric_limits<int>::max()}; };
    std::vector<Candidate> candidates;
    const auto groups = discover_cgroup_groups(4096U, 8U);
    for (const auto& group : groups) {
        if (group.membership == CgroupMembershipInterface::None || group.mount.read_only ||
            (!group.has_process_list && group.membership == CgroupMembershipInterface::Processes)) continue;

        fs::path membership_file;
        if (group.has_process_list) membership_file = fs::path(group.absolute_path) / "cgroup.procs";
        else if (group.membership == CgroupMembershipInterface::Threads)
            membership_file = fs::path(group.absolute_path) / "cgroup.threads";
        else if (group.membership == CgroupMembershipInterface::Tasks)
            membership_file = fs::path(group.absolute_path) / "tasks";
        if (membership_file.empty()) continue;

        const auto ids_text = read_small(membership_file, 32768U);
        if (!ids_text) continue;
        std::istringstream ids(*ids_text);
        std::string token;
        std::set<int> checked_pids;
        int group_best = std::numeric_limits<int>::max();
        bool has_foreground_user_process = false;
        while (ids >> token) {
            uint64_t id_value = 0;
            if (!parse_u64(token, id_value) || id_value == 0U || id_value > 2147483647U) continue;
            const int id = static_cast<int>(id_value);
            const auto status = read_small(fs::path(proc_root_) / token / "status", 16384U);
            if (!status) continue;
            bool app_uid = false;
            int process_id = id;
            std::istringstream status_lines(*status);
            std::string line;
            while (std::getline(status_lines, line)) {
                if (line.rfind("Uid:", 0) == 0) {
                    std::istringstream values(line.substr(4));
                    uint64_t uid = 0;
                    if (values >> uid) app_uid = uid >= 10000U;
                } else if (line.rfind("Tgid:", 0) == 0) {
                    std::istringstream values(line.substr(5));
                    int tgid = 0;
                    if (values >> tgid && tgid > 0) process_id = tgid;
                }
            }
            if (!app_uid || !checked_pids.insert(process_id).second) continue;

            const auto process_token = std::to_string(process_id);
            const auto oom_text = read_small(fs::path(proc_root_) / process_token / "oom_score_adj", 64U);
            if (!oom_text) continue;
            int oom = 1001;
            const auto trimmed_begin = oom_text->find_first_not_of(" \t\r\n");
            if (trimmed_begin == std::string::npos) continue;
            const auto trimmed_end = oom_text->find_last_not_of(" \t\r\n");
            const auto view = std::string_view(*oom_text).substr(trimmed_begin, trimmed_end - trimmed_begin + 1U);
            const auto parsed = std::from_chars(view.data(), view.data() + view.size(), oom);
            if (parsed.ec != std::errc{} || parsed.ptr != view.data() + view.size()) continue;

            // The process leader's own /proc membership must map back to the
            // exact discovered mount and group. A path label is never consulted.
            const auto membership = resolve_process_membership(process_id, group.mount);
            if (!membership || membership->absolute_path != group.absolute_path) continue;
            group_best = std::min(group_best, oom);
            has_foreground_user_process = true;
        }
        if (has_foreground_user_process && group_best <= 0)
            candidates.push_back(Candidate{group, group_best});
    }
    if (candidates.empty()) return std::nullopt;
    const auto best = std::min_element(candidates.begin(), candidates.end(),
        [](const Candidate& a, const Candidate& b) { return a.oom_score < b.oom_score; });
    const auto equal_best = std::count_if(candidates.begin(), candidates.end(), [&](const Candidate& item) {
        return item.oom_score == best->oom_score;
    });
    if (equal_best != 1) return std::nullopt;
    return best->group;
}

namespace {

bool cpu_controller_mount(const CgroupMount& mount) {
    if (mount.filesystem == CgroupFilesystem::V1) {
        return has_token(mount.controllers, "cpu") || has_token(mount.controllers, "cpuctl");
    }
    // cgroup2's mount options do not carry controller names. The controller
    // must be advertised by the mounted hierarchy before it is considered.
    const auto controllers = read_small(fs::path(mount.mount_point) / "cgroup.controllers", 4096U);
    if (!controllers) return false;
    std::istringstream input(*controllers);
    std::string token;
    while (input >> token) if (token == "cpu") return true;
    return false;
}

bool read_app_identity(const std::string& proc_root, const std::string& id,
                       uint64_t& uid) {
    const auto status = read_small(fs::path(proc_root) / id / "status", 16384U);
    if (!status) return false;
    std::istringstream lines(*status);
    std::string line;
    while (std::getline(lines, line)) {
        if (line.rfind("Uid:", 0) != 0) continue;
        std::istringstream values(line.substr(4));
        return static_cast<bool>(values >> uid) && uid >= 10000U;
    }
    return false;
}

bool probe_write_open(const std::string& path, int& error) noexcept {
    const int fd = ::open(path.c_str(), O_WRONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) {
        error = errno;
        return false;
    }
    (void)::close(fd);
    error = 0;
    return true;
}

bool group_has_verified_app_process(const MountTopologyResolver& resolver,
                                    const CgroupGroupCapability& group,
                                    const std::string& proc_root,
                                    uint32_t& count) {
    count = 0;
    fs::path membership_file;
    if (group.has_process_list) membership_file = fs::path(group.absolute_path) / "cgroup.procs";
    else if (group.membership == CgroupMembershipInterface::Threads)
        membership_file = fs::path(group.absolute_path) / "cgroup.threads";
    else if (group.membership == CgroupMembershipInterface::Tasks)
        membership_file = fs::path(group.absolute_path) / "tasks";
    if (membership_file.empty()) return false;

    const auto ids_text = read_small(membership_file, 32768U);
    if (!ids_text) return false;
    std::istringstream ids(*ids_text);
    std::string token;
    std::set<int> checked_ids;
    while (ids >> token) {
        uint64_t parsed_id = 0;
        if (!parse_u64(token, parsed_id) || parsed_id == 0U || parsed_id > 2147483647U) continue;
        const int id = static_cast<int>(parsed_id);
        if (!checked_ids.insert(id).second) continue;
        uint64_t uid = 0;
        if (!read_app_identity(proc_root, token, uid)) continue;
        // Verify the exact PID/TID listed in the group, not only its TGID. This
        // matters on Android's legacy tasks ABI where individual threads move.
        const auto membership = resolver.resolve_process_membership(id, group.mount);
        if (!membership || membership->absolute_path != group.absolute_path) continue;
        ++count;
    }
    return count > 0U;
}

} // namespace

std::optional<CgroupGroupCapability> MountTopologyResolver::resolve_foreground_cpu_group() const {
    const auto groups = discover_cgroup_groups(4096U, 8U, true);
    std::vector<CgroupGroupCapability> candidates;
    for (auto group : groups) {
        if (group.mount.read_only || !cpu_controller_mount(group.mount) ||
            group.membership == CgroupMembershipInterface::None) continue;
        uint32_t app_count = 0;
        if (!group_has_verified_app_process(*this, group, proc_root_, app_count)) continue;
        group.verified_user_processes = app_count;
        candidates.push_back(std::move(group));
    }
    if (candidates.empty()) return std::nullopt;
    std::sort(candidates.begin(), candidates.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.verified_user_processes != rhs.verified_user_processes)
            return lhs.verified_user_processes > rhs.verified_user_processes;
        if (lhs.mount.mount_id != rhs.mount.mount_id) return lhs.mount.mount_id < rhs.mount.mount_id;
        return lhs.absolute_path < rhs.absolute_path;
    });
    if (candidates.size() > 1U &&
        candidates[0].verified_user_processes == candidates[1].verified_user_processes) {
        return std::nullopt;
    }
    return candidates.front();
}

std::optional<CgroupGroupCapability> MountTopologyResolver::resolve_foreground_uclamp_group(
    UclampResolutionStatus* status) const {
    if (status) *status = UclampResolutionStatus::NoCpuCgroupCandidate;
    const auto groups = discover_cgroup_groups(4096U, 8U, true);
    std::vector<CgroupGroupCapability> app_groups;
    std::vector<CgroupGroupCapability> abi_candidates;
    for (auto group : groups) {
        if (group.mount.read_only || !cpu_controller_mount(group.mount) ||
            group.membership == CgroupMembershipInterface::None) continue;
        uint32_t app_count = 0;
        if (!group_has_verified_app_process(*this, group, proc_root_, app_count)) continue;
        group.verified_user_processes = app_count;
        app_groups.push_back(group);
        if (group.has_uclamp_min && group.has_uclamp_max &&
            !group.uclamp_min_path.empty() && !group.uclamp_max_path.empty()) {
            abi_candidates.push_back(std::move(group));
        }
    }
    if (app_groups.empty()) return std::nullopt;
    if (abi_candidates.empty()) {
        if (status) *status = UclampResolutionStatus::UclampAbiUnverified;
        return std::nullopt;
    }

    std::sort(abi_candidates.begin(), abi_candidates.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.verified_user_processes != rhs.verified_user_processes)
            return lhs.verified_user_processes > rhs.verified_user_processes;
        if (lhs.mount.mount_id != rhs.mount.mount_id) return lhs.mount.mount_id < rhs.mount.mount_id;
        return lhs.absolute_path < rhs.absolute_path;
    });
    if (abi_candidates.size() > 1U &&
        abi_candidates[0].verified_user_processes == abi_candidates[1].verified_user_processes) {
        if (status) *status = UclampResolutionStatus::AmbiguousCpuCandidate;
        return std::nullopt;
    }

    auto candidate = std::move(abi_candidates.front());
    int min_error = 0;
    int max_error = 0;
    const bool min_open = probe_write_open(candidate.uclamp_min_path, min_error);
    const bool max_open = probe_write_open(candidate.uclamp_max_path, max_error);
    candidate.uclamp_write_open_verified = min_open && max_open;
    candidate.uclamp_write_open_errno = !min_open ? min_error : max_error;
    if (!candidate.uclamp_write_open_verified) {
        if (status) *status = UclampResolutionStatus::WriteOpenDenied;
        return std::nullopt;
    }
    if (status) *status = UclampResolutionStatus::AccessVerified;
    return candidate;
}

const char* MountTopologyResolver::uclamp_resolution_status_name(
    UclampResolutionStatus status) noexcept {
    switch (status) {
        case UclampResolutionStatus::NotEvaluated: return "NOT_EVALUATED";
        case UclampResolutionStatus::NoCpuCgroupCandidate: return "NO_CPU_CGROUP_CANDIDATE";
        case UclampResolutionStatus::AmbiguousCpuCandidate: return "AMBIGUOUS_CPU_CANDIDATE";
        case UclampResolutionStatus::UclampAbiUnverified: return "UCLAMP_ABI_UNVERIFIED";
        case UclampResolutionStatus::BaselineCaptureFailed: return "BASELINE_CAPTURE_FAILED";
        case UclampResolutionStatus::WriteOpenDenied: return "WRITE_OPEN_DENIED";
        case UclampResolutionStatus::SelinuxAccessDenied: return "SELINUX_ACCESS_DENIED";
        case UclampResolutionStatus::AccessVerified: return "ACCESS_VERIFIED";
        case UclampResolutionStatus::MutationEligible: return "MUTATION_ELIGIBLE";
    }
    return "NOT_EVALUATED";
}

std::optional<CgroupGroupCapability> MountTopologyResolver::resolve_foreground_io_group() const {
    const auto foreground = resolve_foreground_group();
    if (!foreground || !foreground->has_io_weight || foreground->io_weight_path.empty() ||
        foreground->mount.read_only) return std::nullopt;
    return foreground;
}

std::optional<ParsedUclampValue> MountTopologyResolver::parse_uclamp_value(std::string_view raw) noexcept {
    const auto first = raw.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) return std::nullopt;
    const auto last = raw.find_last_not_of(" \t\r\n");
    raw = raw.substr(first, last - first + 1U);
    if (raw == "max") return ParsedUclampValue{1024U, UclampEncoding::Percent, true};

    double value = 0.0;
    const auto parsed = std::from_chars(raw.data(), raw.data() + raw.size(), value, std::chars_format::general);
    if (parsed.ec != std::errc{} || parsed.ptr != raw.data() + raw.size() ||
        !std::isfinite(value) || value < 0.0) return std::nullopt;
    const bool percent = raw.find('.') != std::string_view::npos;
    if (percent) {
        if (value > 100.0) return std::nullopt;
        const auto capacity = static_cast<uint32_t>(std::llround(value * 10.24));
        return ParsedUclampValue{std::min(capacity, 1024U), UclampEncoding::Percent, false};
    }
    if (value > 1024.0 || std::floor(value) != value) return std::nullopt;
    return ParsedUclampValue{static_cast<uint32_t>(value), UclampEncoding::Capacity1024, false};
}

std::string MountTopologyResolver::encode_uclamp_value(
    uint32_t capacity, UclampEncoding encoding, bool maximum) {
    capacity = std::min(capacity, 1024U);
    if (encoding == UclampEncoding::Capacity1024) return std::to_string(capacity);
    if (maximum && capacity == 1024U) return "max";
    const uint32_t hundredths = (capacity * 10000U + 512U) / 1024U;
    std::ostringstream out;
    out << (hundredths / 100U) << '.';
    const auto fraction = hundredths % 100U;
    if (fraction < 10U) out << '0';
    out << fraction;
    return out.str();
}

const char* MountTopologyResolver::membership_name(CgroupMembershipInterface value) noexcept {
    switch (value) {
        case CgroupMembershipInterface::Threads: return "cgroup.threads";
        case CgroupMembershipInterface::Tasks: return "tasks";
        case CgroupMembershipInterface::Processes: return "cgroup.procs";
        case CgroupMembershipInterface::None: return "none";
    }
    return "none";
}

} // namespace coreflow
