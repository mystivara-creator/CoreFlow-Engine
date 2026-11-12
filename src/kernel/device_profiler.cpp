/**
 * @file device_profiler.cpp
 * @brief Read-only, root-injectable OS capability discovery.
 * @license Apache-2.0
 */
#include "coreflow/device_profiler.hpp"
#include "coreflow/baseline_store.hpp"
#include "coreflow/mount_topology.hpp"
#include "coreflow/kernel_node_inventory.hpp"
#include "coreflow/thermal_monitor.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cctype>
#include <cerrno>
#include <filesystem>
#include <fcntl.h>
#include <iomanip>
#include <sstream>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>

namespace coreflow {
namespace fs = std::filesystem;
namespace {
constexpr size_t kMaxAttributeBytes = 64U * 1024U;
constexpr size_t kMaxDirectoryEntries = 256U;
constexpr uint32_t kMaxCpuId = 4095U;
constexpr uint64_t kMaxFrequencyEntries = 512U;

std::string trim(std::string value) {
    const size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1U);
}

bool read_attribute(const fs::path& path, std::string& out, size_t limit = kMaxAttributeBytes) {
    out.clear();
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return false;
    struct stat st{};
    if (::fstat(fd, &st) != 0 || !S_ISREG(st.st_mode)) { (void)::close(fd); return false; }
    std::array<char, 4096> buffer{};
    bool ok = true;
    for (;;) {
        const ssize_t n = ::read(fd, buffer.data(), buffer.size());
        if (n < 0 && errno == EINTR) continue;
        if (n < 0) { ok = false; break; }
        if (n == 0) break;
        const size_t count = static_cast<size_t>(n);
        if (out.size() > limit || count > limit - out.size()) { ok = false; break; }
        out.append(buffer.data(), count);
    }
    (void)::close(fd);
    if (!ok) out.clear();
    return ok && !out.empty();
}
std::string read_node_value(const std::string& path) {
    std::string value;
    if (!read_attribute(fs::path(path), value, 1024U)) return {};
    return trim(std::move(value));
}
bool parse_u64(std::string_view text, uint64_t& out) noexcept {
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())) != 0) text.remove_prefix(1);
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())) != 0) text.remove_suffix(1);
    if (text.empty()) return false;
    uint64_t value = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value, 10);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) return false;
    out = value;
    return true;
}
std::vector<std::string> parse_tokens(std::string_view text) {
    std::vector<std::string> tokens;
    std::istringstream input{std::string(text)};
    std::string token;
    while (input >> token) {
        if (token.size() > 128U) continue;
        if (token.front() == '[' && token.back() == ']') token = token.substr(1U, token.size() - 2U);
        if (!token.empty() && std::find(tokens.begin(), tokens.end(), token) == tokens.end())
            tokens.push_back(std::move(token));
    }
    return tokens;
}
std::vector<std::string> regular_attribute_names(const fs::path& directory,
                                                  size_t maximum = 192U) {
    std::vector<std::string> names;
    std::error_code ec;
    fs::directory_iterator it(directory, fs::directory_options::skip_permission_denied, ec);
    const fs::directory_iterator end;
    for (; !ec && it != end && names.size() < maximum; it.increment(ec)) {
        std::error_code status_ec;
        const auto status = it->symlink_status(status_ec);
        if (status_ec || fs::is_symlink(status) || !fs::is_regular_file(status)) continue;
        names.push_back(it->path().filename().string());
    }
    std::sort(names.begin(), names.end());
    return names;
}
std::string active_bracketed_token(std::string_view text) {
    const auto open = text.find('[');
    const auto close = open == std::string_view::npos ? std::string_view::npos : text.find(']', open + 1U);
    if (open != std::string_view::npos && close != std::string_view::npos)
        return std::string(text.substr(open + 1U, close - open - 1U));
    return std::string(text);
}
bool parse_frequency_table(std::string_view text, std::vector<uint64_t>& out) {
    out.clear();
    std::istringstream input{std::string(text)};
    std::string token;
    while (input >> token) {
        uint64_t value = 0;
        if (!parse_u64(token, value) || value == 0 || out.size() >= kMaxFrequencyEntries) {
            out.clear(); return false;
        }
        out.push_back(value);
    }
    if (out.empty()) return false;
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return !out.empty();
}
bool read_u64(const fs::path& path, uint64_t& value) {
    std::string raw;
    return read_attribute(path, raw, 256U) && parse_u64(raw, value);
}
bool cddp_path_exists(const fs::path& path) noexcept {
    struct stat st{};
    return ::stat(path.c_str(), &st) == 0;
}
bool writable_by_mode_and_process(const fs::path& path) noexcept {
    struct stat st{};
    if (::stat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) return false;
    // access(W_OK) alone is insufficient under uid 0.
    const bool mode_writable = (st.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)) != 0;
    return mode_writable && ::access(path.c_str(), W_OK) == 0;
}
std::vector<fs::path> directory_entries(const fs::path& root, std::string_view prefix = {}) {
    std::vector<fs::path> result;
    std::error_code ec;
    fs::directory_iterator it(root, fs::directory_options::skip_permission_denied, ec);
    const fs::directory_iterator end;
    for (; !ec && it != end && result.size() < kMaxDirectoryEntries; it.increment(ec)) {
        const std::string name = it->path().filename().string();
        if (!prefix.empty() && name.rfind(prefix, 0) != 0) continue;
        std::error_code type_ec;
        if (fs::is_directory(it->path(), type_ec) && !type_ec) result.push_back(it->path());
    }
    std::sort(result.begin(), result.end());
    return result;
}
uint64_t fnv1a(std::string_view text) noexcept {
    uint64_t hash = 14695981039346656037ULL;
    for (const unsigned char c : text) { hash ^= static_cast<uint64_t>(c); hash *= 1099511628211ULL; }
    return hash;
}
std::string make_fingerprint(const DeviceProfileSnapshot& profile) {
    std::ostringstream data;
    data << "cddp-v1|" << profile.kernel_release << '|' << profile.device_model << '|'
         << profile.soc_model << '|' << profile.hardware_platform << '|';
    for (const auto cpu : profile.possible_cpus) data << "p" << cpu << ',';
    for (const auto cpu : profile.present_cpus) data << "r" << cpu << ',';
    for (const auto cpu : profile.online_cpus) data << "o" << cpu << ',';
    for (const auto& policy : profile.cpu_policies) {
        data << "cpu:" << policy.policy_path << ':' << policy.governor << ':'
             << policy.cluster_role << ':' << policy.capacity_rank << ':';
        for (const auto cpu : policy.related_cpus) data << cpu << ',';
        data << policy.cpuinfo_min_freq_khz << ':' << policy.cpuinfo_max_freq_khz << ':';
        for (const auto freq : policy.available_frequencies_khz) data << freq << ',';
        for (const auto& governor : policy.available_governors) data << "g=" << governor << ',';
        for (const auto& tunable : policy.governor_tunables) data << "t=" << tunable << ',';
    }
    for (const auto& block : profile.block_devices) {
        data << "blk:" << block.device_name << ':' << block.transport_hint << ':'
             << block.capacity_bytes << ':' << block.hardware_queues << ':'
             << block.rotational << ':' << block.active_scheduler << '|';
        for (const auto& scheduler : block.available_schedulers) data << scheduler << ',';
    }
    data << "ram:" << profile.memory_hardware.total_ram_bytes << '|';
    for (const auto& zram : profile.memory_hardware.zram_devices) {
        data << "zram:" << zram.device_name << ':' << zram.disksize_bytes << ':'
             << zram.active_algorithm << ':' << zram.swap_active << '|';
        for (const auto& algorithm : zram.available_algorithms) data << algorithm << ',';
    }
    for (const auto& gpu : profile.gpu_domains) {
        data << "gpu:" << gpu.device_path << ':' << gpu.driver << ':';
        for (const auto freq : gpu.available_frequencies_hz) data << freq << ',';
    }
    for (const auto& cap : profile.node_capabilities) {
        data << "cap:" << cap.path << ':' << cap.semantic_id << ':'
             << static_cast<unsigned>(cap.state) << ':' << cap.unit << ':' << cap.constraints << '|';
    }
    std::ostringstream out;
    out << std::hex << std::setw(16) << std::setfill('0') << fnv1a(data.str());
    return out.str();
}
NodeCapability observe_capability(
    const fs::path& path, HardwareDomain domain, ProfileSource source,
    std::string semantic, std::string unit, std::string constraints,
    bool semantics_known, bool constraints_valid, bool eligible, std::string evidence) {
    NodeCapability cap{};
    cap.path = path.string();
    cap.domain = domain;
    cap.source = source;
    cap.semantic_id = std::move(semantic);
    cap.unit = std::move(unit);
    cap.constraints = std::move(constraints);
    cap.evidence = std::move(evidence);
    if (!cddp_path_exists(path)) { cap.rejection_reason = "node absent"; cap.rejection_code = NodeRejectionReason::NodeMissing; return cap; }
    cap.state = NodeCapabilityState::Available;
    std::string raw;
    if (!read_attribute(path, raw, 1024U)) {
        cap.rejection_reason = "node not readable as a bounded regular attribute"; cap.rejection_code = NodeRejectionReason::IoError; return cap;
    }
    cap.current_value = trim(raw);
    cap.state = NodeCapabilityState::Readable;
    if (!writable_by_mode_and_process(path)) {
        cap.rejection_reason = "write permission not established by mode and process access"; cap.rejection_code = NodeRejectionReason::PermissionDenied; return cap;
    }
    cap.state = NodeCapabilityState::Writable;
    if (!semantics_known || cap.semantic_id.empty() || cap.unit.empty()) {
        cap.rejection_reason = "interface semantics or unit are not verified"; cap.rejection_code = NodeRejectionReason::SemanticsUnverified; return cap;
    }
    cap.state = NodeCapabilityState::SemanticsVerified;
    if (!constraints_valid || cap.constraints.empty()) {
        cap.rejection_reason = "accepted value constraints or preflight are not verified"; cap.rejection_code = NodeRejectionReason::ConstraintsUnverified; return cap;
    }
    cap.state = NodeCapabilityState::ConstraintsVerified;
    if (!eligible) {
        cap.rejection_reason = "static checks passed but runtime mutation admission was not granted";
        cap.rejection_code = NodeRejectionReason::BaselineNotCaptured;
        return cap;
    }
    auto& baseline = BaselineStore::instance();
    if (!baseline.capture(path.string())) {
        cap.rejection_reason = "OEM baseline capture failed during passive discovery";
        cap.rejection_code = NodeRejectionReason::BaselineCaptureFailed;
        return cap;
    }
    const auto audit = baseline.get_audit_trail();
    const auto captured = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& node) {
        return node.path == path.string() && !node.is_modified && node.factory_value == cap.current_value;
    });
    if (captured == audit.end()) {
        cap.rejection_reason = "baseline is missing, modified, or differs from the observed value";
        cap.rejection_code = NodeRejectionReason::BaselineNotCaptured;
        return cap;
    }
    cap.state = NodeCapabilityState::MutationEligible;
    cap.rejection_code = NodeRejectionReason::None;
    return cap;
}
bool valid_subset(const std::vector<uint32_t>& subset, const std::vector<uint32_t>& superset) {
    return std::all_of(subset.begin(), subset.end(), [&](uint32_t cpu) {
        return std::binary_search(superset.begin(), superset.end(), cpu);
    });
}
std::string join_cpu_ids(const std::vector<uint32_t>& cpus) {
    std::ostringstream out;
    for (size_t i = 0; i < cpus.size(); ++i) { if (i != 0) out << ','; out << cpus[i]; }
    return out.str();
}
} // namespace

DeviceProfiler::DeviceProfiler(std::string sys_root, std::string proc_root)
    : sys_root_(std::move(sys_root)), proc_root_(std::move(proc_root)) {
    if (sys_root_.empty()) sys_root_ = "/sys";
    if (proc_root_.empty()) proc_root_ = "/proc";
}
bool DeviceProfiler::parse_cpu_list(std::string_view text, std::vector<uint32_t>& cpus) noexcept {
    cpus.clear();
    if (text.empty() || text.size() > 16384U) return false;

    // Linux cpulist attributes commonly use comma-separated ranges (possible/present),
    // while cpufreq related_cpus/affected_cpus commonly use whitespace-separated IDs.
    // Accept either delimiter, including mixed forms, but reject empty comma fields.
    size_t pos = 0;
    bool comma_requires_token = false;
    while (pos < text.size()) {
        while (pos < text.size() &&
               std::isspace(static_cast<unsigned char>(text[pos])) != 0) ++pos;
        if (pos == text.size()) break;
        if (text[pos] == ',') {
            if (cpus.empty() || comma_requires_token) { cpus.clear(); return false; }
            comma_requires_token = true;
            ++pos;
            continue;
        }

        const size_t begin = pos;
        while (pos < text.size() && text[pos] != ',' &&
               std::isspace(static_cast<unsigned char>(text[pos])) == 0) ++pos;
        const std::string_view item = text.substr(begin, pos - begin);
        if (item.empty()) { cpus.clear(); return false; }

        const size_t dash = item.find('-');
        uint64_t first = 0, last = 0;
        if (dash == std::string_view::npos) {
            if (!parse_u64(item, first)) { cpus.clear(); return false; }
            last = first;
        } else {
            if (item.find('-', dash + 1U) != std::string_view::npos ||
                !parse_u64(item.substr(0, dash), first) ||
                !parse_u64(item.substr(dash + 1U), last) || first > last) {
                cpus.clear(); return false;
            }
        }

        if (last > kMaxCpuId || (last - first) > kMaxCpuId ||
            cpus.size() + (last - first + 1U) > 4096U) { cpus.clear(); return false; }
        for (uint64_t cpu = first; cpu <= last; ++cpu) cpus.push_back(static_cast<uint32_t>(cpu));
        comma_requires_token = false;
    }
    if (comma_requires_token) { cpus.clear(); return false; }

    std::sort(cpus.begin(), cpus.end());
    if (cpus.empty() || std::adjacent_find(cpus.begin(), cpus.end()) != cpus.end()) {
        cpus.clear();
        return false;
    }
    return true;
}
DeviceProfileHandle DeviceProfiler::profile() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!current_) { ++generation_; current_ = build_profile(generation_); }
    return current_;
}
DeviceProfileHandle DeviceProfiler::refresh() {
    std::lock_guard<std::mutex> lock(mutex_);
    ++generation_;
    current_ = build_profile(generation_);
    return current_;
}
DeviceProfileHandle DeviceProfiler::build_profile(uint64_t generation) const {
    auto snapshot = std::make_shared<DeviceProfileSnapshot>();
    snapshot->generation = generation;
    snapshot->sys_root = sys_root_;
    snapshot->proc_root = proc_root_;
    snapshot->captured_monotonic_ns = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    std::string raw;
    const fs::path sys(sys_root_);
    const fs::path proc(proc_root_);

    // Root injection is hermetic: never substitute the host kernel identity
    // when the injected procfs tree does not expose a release value.
    if (read_attribute(proc / "sys/kernel/osrelease", raw, 256U)) {
        snapshot->kernel_release = trim(raw);
    }

    if (read_attribute(sys / "firmware/devicetree/base/model", raw) ||
        read_attribute(proc / "device-tree/model", raw)) {
        while (!raw.empty() && raw.back() == '\0') raw.pop_back();
        snapshot->device_model = trim(raw);
    }
    if (read_attribute(sys / "devices/soc0/machine", raw, 1024U)) snapshot->soc_model = trim(raw);
    if (read_attribute(sys / "devices/soc0/family", raw, 1024U)) snapshot->hardware_platform = trim(raw);

    const auto read_cpu_set = [&](const char* name, std::vector<uint32_t>& values) {
        std::string content;
        return read_attribute(sys / "devices/system/cpu" / name, content, 16384U) &&
               parse_cpu_list(content, values);
    };
    const bool possible_ok = read_cpu_set("possible", snapshot->possible_cpus);
    const bool present_ok = read_cpu_set("present", snapshot->present_cpus);
    const bool online_ok = read_cpu_set("online", snapshot->online_cpus);
    snapshot->cpu_topology_valid = possible_ok && present_ok && online_ok &&
        valid_subset(snapshot->present_cpus, snapshot->possible_cpus) &&
        valid_subset(snapshot->online_cpus, snapshot->present_cpus);

    const fs::path cpu_root = sys / "devices/system/cpu/cpufreq";
    for (const auto& dir : directory_entries(cpu_root, "policy")) {
        CpuPolicyProfile policy{};
        policy.policy_path = dir.string();
        bool membership_parsed = false;
        if (read_attribute(dir / "related_cpus", raw, 16384U)) {
            membership_parsed = parse_cpu_list(raw, policy.related_cpus);
        }
        if (!membership_parsed && read_attribute(dir / "affected_cpus", raw, 16384U)) {
            membership_parsed = parse_cpu_list(raw, policy.related_cpus);
        }
        policy.related_cpus_valid = membership_parsed &&
            (!snapshot->cpu_topology_valid || valid_subset(policy.related_cpus, snapshot->present_cpus));
        (void)read_u64(dir / "cpuinfo_min_freq", policy.cpuinfo_min_freq_khz);
        (void)read_u64(dir / "cpuinfo_max_freq", policy.cpuinfo_max_freq_khz);
        (void)read_u64(dir / "scaling_min_freq", policy.scaling_min_freq_khz);
        (void)read_u64(dir / "scaling_max_freq", policy.scaling_max_freq_khz);
        (void)read_u64(dir / "scaling_cur_freq", policy.scaling_cur_freq_khz);
        if (read_attribute(dir / "scaling_governor", raw, 128U)) policy.governor = trim(raw);
        if (read_attribute(dir / "scaling_available_governors", raw, 4096U))
            policy.available_governors = parse_tokens(raw);
        if (!policy.governor.empty())
            policy.governor_tunables = regular_attribute_names(dir / policy.governor);
        std::vector<uint64_t> table;
        if (read_attribute(dir / "scaling_available_frequencies", raw, 8192U) ||
            read_attribute(dir / "cpuinfo_available_frequencies", raw, 8192U)) {
            policy.frequency_table_valid = parse_frequency_table(raw, table);
            if (policy.frequency_table_valid) policy.available_frequencies_khz = table;
        }
        const bool valid_bounds = policy.cpuinfo_min_freq_khz > 0 &&
            policy.cpuinfo_max_freq_khz >= policy.cpuinfo_min_freq_khz &&
            policy.scaling_min_freq_khz >= policy.cpuinfo_min_freq_khz &&
            policy.scaling_max_freq_khz <= policy.cpuinfo_max_freq_khz &&
            policy.scaling_min_freq_khz <= policy.scaling_max_freq_khz;
        const bool table_contains_active = policy.frequency_table_valid &&
            std::binary_search(policy.available_frequencies_khz.begin(),
                               policy.available_frequencies_khz.end(), policy.scaling_max_freq_khz);
        const fs::path max_path = dir / "scaling_max_freq";
        const bool eligible = snapshot->cpu_topology_valid && policy.related_cpus_valid &&
            valid_bounds && table_contains_active && policy.scaling_max_freq_khz > 0;
        auto cap = observe_capability(max_path, HardwareDomain::Cpu, ProfileSource::Sysfs,
            "cpufreq.scaling_max_freq", "kHz",
            "cpuinfo_min_freq<=target<=cpuinfo_max_freq;target in scaling_available_frequencies",
            true, valid_bounds && table_contains_active, eligible,
            "Linux CPUFreq sysfs ABI; exact policy membership, bounds and advertised table checked");
        if (!eligible && cap.rejection_reason.empty())
            cap.rejection_reason = "CPU topology, policy membership, bounds or frequency table preflight failed";
        snapshot->node_capabilities.push_back(std::move(cap));

        // Governor-specific rate-limit controls are observed but never promoted
        // without a verified driver-specific ABI and accepted range contract.
        if (!policy.governor.empty()) {
            const fs::path governor_dir = dir / policy.governor;
            for (const auto& tunable : regular_attribute_names(governor_dir)) {
                const fs::path path = governor_dir / tunable;
                NodeCapability governor_cap{};
                governor_cap.path = path.string();
                governor_cap.domain = HardwareDomain::Cpu;
                governor_cap.source = ProfileSource::Sysfs;
                governor_cap.semantic_id = "cpufreq.governor_tunable.unverified";
                governor_cap.parent_policy_path = (dir / "scaling_governor").string();
                governor_cap.parent_policy_value = policy.governor;
                governor_cap.dependency_group = dir.filename().string() + ":governor";
                governor_cap.rediscover_after_parent_change = true;
                governor_cap.evidence = "Active governor child enumerated dynamically; driver ABI and accepted range not inferred";
                if (read_attribute(path, raw, 256U)) {
                    governor_cap.current_value = trim(raw);
                    governor_cap.state = NodeCapabilityState::Readable;
                    if (writable_by_mode_and_process(path)) governor_cap.state = NodeCapabilityState::Writable;
                }
                governor_cap.rejection_reason = "Governor child tunable is observation-only until a driver-specific contract is verified";
                governor_cap.rejection_code = NodeRejectionReason::SemanticsUnverified;
                snapshot->node_capabilities.push_back(std::move(governor_cap));
            }
        }
        snapshot->cpu_policies.push_back(std::move(policy));
    }

    // Rank independently discovered cpufreq policies by their advertised maximum
    // capacity. Names such as little/big/prime are descriptive roles, never
    // assumptions about policy indices or SoC cluster count.
    std::vector<std::size_t> ranked_policies;
    ranked_policies.reserve(snapshot->cpu_policies.size());
    for (std::size_t i = 0; i < snapshot->cpu_policies.size(); ++i) {
        if (snapshot->cpu_policies[i].cpuinfo_max_freq_khz > 0U)
            ranked_policies.push_back(i);
    }
    std::sort(ranked_policies.begin(), ranked_policies.end(), [&](std::size_t lhs, std::size_t rhs) {
        const auto& a = snapshot->cpu_policies[lhs];
        const auto& b = snapshot->cpu_policies[rhs];
        if (a.cpuinfo_max_freq_khz != b.cpuinfo_max_freq_khz)
            return a.cpuinfo_max_freq_khz < b.cpuinfo_max_freq_khz;
        return a.policy_path < b.policy_path;
    });
    for (std::size_t rank = 0; rank < ranked_policies.size(); ++rank) {
        auto& policy = snapshot->cpu_policies[ranked_policies[rank]];
        policy.capacity_rank = static_cast<uint32_t>(rank);
        if (ranked_policies.size() == 1U) policy.cluster_role = "unified";
        else if (rank == 0U) policy.cluster_role = "little";
        else if (rank + 1U == ranked_policies.size()) policy.cluster_role = "prime_or_big";
        else if (rank == 1U) policy.cluster_role = "mid";
        else policy.cluster_role = "big";
    }

    const fs::path devfreq_root = sys / "class/devfreq";
    for (const auto& dir : directory_entries(devfreq_root)) {
        const std::string name = dir.filename().string();
        std::string driver;
        if (read_attribute(dir / "name", raw, 256U)) driver = trim(raw);
        if (driver.empty()) driver = name;
        std::string lowered = name + " " + driver;
        std::transform(lowered.begin(), lowered.end(), lowered.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (lowered.find("gpu") == std::string::npos &&
            lowered.find("mali") == std::string::npos &&
            lowered.find("adreno") == std::string::npos &&
            lowered.find("kgsl") == std::string::npos) continue;

        GpuDomainProfile gpu{};
        gpu.device_path = dir.string();
        gpu.driver = driver;
        (void)read_u64(dir / "cur_freq", gpu.cur_freq_hz);
        (void)read_u64(dir / "min_freq", gpu.min_freq_hz);
        (void)read_u64(dir / "max_freq", gpu.max_freq_hz);
        std::vector<uint64_t> table;
        if (read_attribute(dir / "available_frequencies", raw, 8192U)) {
            gpu.frequency_table_valid = parse_frequency_table(raw, table);
            if (gpu.frequency_table_valid) gpu.available_frequencies_hz = table;
        }
        if (read_attribute(dir / "governor", raw, 128U)) gpu.governor = trim(raw);
        const bool recognized_driver = lowered.find("mali") != std::string::npos ||
            lowered.find("adreno") != std::string::npos || lowered.find("gpu") != std::string::npos;
        const bool valid_bounds = gpu.min_freq_hz > 0 && gpu.max_freq_hz >= gpu.min_freq_hz;
        const bool table_contains_active = gpu.frequency_table_valid &&
            std::binary_search(gpu.available_frequencies_hz.begin(),
                               gpu.available_frequencies_hz.end(), gpu.max_freq_hz);
        const bool eligible = recognized_driver && valid_bounds && table_contains_active && gpu.max_freq_hz > 0;
        if (recognized_driver) {
            auto cap = observe_capability(dir / "max_freq", HardwareDomain::Gpu, ProfileSource::Sysfs,
                "devfreq.max_freq", "Hz",
                "min_freq<=target<=max_freq;target in available_frequencies",
                true, valid_bounds && table_contains_active, eligible,
                "Linux devfreq generic ABI; GPU identity, bounds and advertised frequency table checked");
            if (!eligible && cap.rejection_reason.empty())
                cap.rejection_reason = "GPU identity, bounds or frequency table preflight failed";
            snapshot->node_capabilities.push_back(std::move(cap));
        }
        snapshot->gpu_domains.push_back(std::move(gpu));
    }

    // Vendor GPU classes are enumerated dynamically. Unknown attributes remain
    // visible as observation-only rather than being guessed as controls.
    const fs::path kgsl_root = sys / "class/kgsl";
    for (const auto& kgsl : directory_entries(kgsl_root)) {
        GpuDomainProfile gpu{};
        gpu.device_path = kgsl.string();
        std::string driver_name;
        if (read_attribute(kgsl / "name", driver_name, 256U)) driver_name = trim(driver_name);
        gpu.driver = driver_name.empty() ? kgsl.filename().string() : driver_name;
        snapshot->gpu_domains.push_back(std::move(gpu));
        std::error_code iter_ec;
        for (fs::directory_iterator it(kgsl, fs::directory_options::skip_permission_denied, iter_ec), end;
             !iter_ec && it != end; it.increment(iter_ec)) {
            std::error_code type_ec;
            const auto status = it->symlink_status(type_ec);
            if (type_ec || fs::is_symlink(status) || !fs::is_regular_file(status)) continue;
            NodeCapability cap{};
            cap.path = it->path().string();
            cap.domain = HardwareDomain::Gpu;
            cap.source = ProfileSource::Sysfs;
            cap.semantic_id = "kgsl.vendor_attribute.unverified";
            cap.evidence = "Discovered through the runtime GPU class directory; vendor ABI is not yet verified";
            std::string value;
            if (read_attribute(it->path(), value, 256U)) {
                cap.current_value = trim(value);
                cap.state = NodeCapabilityState::Readable;
                cap.rejection_reason = "vendor-specific control is observation-only until its ABI and constraints are verified";
                cap.rejection_code = NodeRejectionReason::SemanticsUnverified;
            } else {
                cap.state = NodeCapabilityState::Discovered;
                cap.rejection_reason = "discovered GPU attribute could not be read";
                cap.rejection_code = NodeRejectionReason::IoError;
            }
            snapshot->node_capabilities.push_back(std::move(cap));
        }
    }

    const fs::path thermal_root = sys / "class/thermal";
    for (const auto& zone : directory_entries(thermal_root, "thermal_zone")) {
        ThermalSensorProfile sensor{};
        sensor.sensor_path = zone.string();
        if (read_attribute(zone / "type", raw, 256U)) sensor.sensor_type = trim(raw);
        uint64_t temp = 0;
        if (read_u64(zone / "temp", temp) && temp <= 200000U) {
            sensor.temperature_millicelsius = static_cast<int64_t>(temp);
            sensor.temperature_valid = true;
        }
        snapshot->thermal_sensors.push_back(std::move(sensor));
    }
    const fs::path hwmon_root = sys / "class/hwmon";
    for (const auto& hwmon : directory_entries(hwmon_root, "hwmon")) {
        std::string chip;
        if (read_attribute(hwmon / "name", chip, 256U)) chip = trim(chip);
        std::error_code ec;
        for (fs::directory_iterator it(hwmon, fs::directory_options::skip_permission_denied, ec), end;
             !ec && it != end; it.increment(ec)) {
            const std::string filename = it->path().filename().string();
            if (filename.rfind("temp", 0) != 0 || filename.find("_input") == std::string::npos) continue;
            uint64_t temp = 0;
            if (!read_u64(it->path(), temp) || temp > 200000U) continue;
            ThermalSensorProfile sensor{};
            sensor.sensor_path = it->path().string();
            sensor.sensor_type = chip.empty() ? filename : chip + "/" + filename;
            sensor.temperature_millicelsius = static_cast<int64_t>(temp);
            sensor.temperature_valid = true;
            snapshot->thermal_sensors.push_back(std::move(sensor));
        }
    }

    // Read-only hardware specification snapshot: storage queue topology, RAM,
    // and active/inactive ZRAM ABI are discovered from the live kernel tree.
    const fs::path block_class_root = sys / "class/block";
    for (const auto& block : directory_entries(block_class_root)) {
        BlockDeviceProfile surface{};
        surface.device_name = block.filename().string();
        surface.device_path = block.string();
        const fs::path queue = block / "queue";
        uint64_t value = 0;
        if (read_u64(queue / "nr_hw_queues", value) && value <= UINT32_MAX)
            surface.hardware_queues = static_cast<uint32_t>(value);
        if (read_u64(queue / "rotational", value)) {
            surface.rotational = value != 0U;
            surface.rotational_valid = value <= 1U;
        }
        if (read_u64(block / "size", value) && value <= UINT64_MAX / 512U)
            surface.capacity_bytes = value * 512U;
        (void)read_attribute(block / "device/model", raw, 256U);
        surface.model = trim(raw);
        raw.clear();
        (void)read_attribute(block / "device/vendor", raw, 256U);
        surface.vendor = trim(raw);
        std::error_code canonical_error;
        const auto canonical = fs::weakly_canonical(block, canonical_error).string();
        std::string identity = canonical_error ? block.string() : canonical;
        std::transform(identity.begin(), identity.end(), identity.begin(),
            [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        if (identity.find("ufs") != std::string::npos) surface.transport_hint = "ufs";
        else if (identity.find("mmc") != std::string::npos) surface.transport_hint = "emmc_or_mmc";
        else if (surface.rotational_valid) surface.transport_hint = surface.rotational ? "rotational" : "non_rotational_flash";
        raw.clear();
        if (read_attribute(queue / "scheduler", raw, 4096U)) {
            surface.active_scheduler = active_bracketed_token(raw);
            surface.available_schedulers = parse_tokens(raw);
        }
        surface.queue_tunables = regular_attribute_names(queue);
        surface.iosched_tunables = regular_attribute_names(queue / "iosched");
        snapshot->block_devices.push_back(std::move(surface));
    }

    // MemTotal is reported in KiB by procfs; convert with overflow checking.
    if (read_attribute(proc / "meminfo", raw, 64U * 1024U)) {
        std::istringstream lines(raw);
        std::string line;
        while (std::getline(lines, line)) {
            if (line.rfind("MemTotal:", 0) != 0) continue;
            std::istringstream fields(line.substr(9));
            uint64_t kib = 0;
            if (fields >> kib && kib <= UINT64_MAX / 1024U)
                snapshot->memory_hardware.total_ram_bytes = kib * 1024U;
            break;
        }
    }
    std::string swaps_text;
    (void)read_attribute(proc / "swaps", swaps_text, 64U * 1024U);
    const fs::path zram_root = sys / "block";
    for (const auto& zram : directory_entries(zram_root, "zram")) {
        ZramDeviceProfile device{};
        device.device_name = zram.filename().string();
        device.device_path = zram.string();
        (void)read_u64(zram / "disksize", device.disksize_bytes);
        raw.clear();
        if (read_attribute(zram / "comp_algorithm", raw, 4096U)) {
            device.active_algorithm = active_bracketed_token(raw);
            device.available_algorithms = parse_tokens(raw);
        }
        device.swap_active = swaps_text.find(device.device_name) != std::string::npos;
        snapshot->memory_hardware.zram_devices.push_back(std::move(device));
    }

    // Enumerate all VM and block-queue attributes from their subsystem roots.
    // A small ABI contract map promotes only known scalar interfaces; every
    // unknown node is still exposed as observation-only inventory.
    const auto scan_observation = [&](const fs::path& path, HardwareDomain domain,
                                      ProfileSource source, std::string semantic,
                                      std::string evidence) {
        ++snapshot->infrastructure_nodes_scanned;
        NodeCapability cap{};
        cap.path = path.string();
        cap.domain = domain;
        cap.source = source;
        cap.semantic_id = std::move(semantic);
        cap.evidence = std::move(evidence);
        struct stat st{};
        if (::lstat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode) || S_ISLNK(st.st_mode)) {
            cap.rejection_reason = "node absent or not a regular ABI attribute";
            cap.rejection_code = NodeRejectionReason::NodeMissing;
            snapshot->node_capabilities.push_back(std::move(cap));
            return;
        }
        cap.state = NodeCapabilityState::Discovered;
        std::string value;
        if (!read_attribute(path, value, 4096U)) {
            cap.rejection_reason = "node exists but bounded read failed";
            cap.rejection_code = NodeRejectionReason::IoError;
            snapshot->node_capabilities.push_back(std::move(cap));
            return;
        }
        cap.current_value = trim(value);
        cap.state = NodeCapabilityState::Readable;
        if (writable_by_mode_and_process(path)) {
            cap.state = NodeCapabilityState::AccessChecked;
            ++snapshot->infrastructure_nodes_writable;
            cap.rejection_reason = "access is visible, but mutation remains disabled without a verified ABI contract and baseline";
            cap.rejection_code = NodeRejectionReason::SemanticsUnverified;
        } else {
            cap.rejection_reason = "readable node; write access not established";
            cap.rejection_code = NodeRejectionReason::PermissionDenied;
        }
        snapshot->node_capabilities.push_back(std::move(cap));
    };

    const auto scan_scalar = [&](const fs::path& path, HardwareDomain domain,
                                 ProfileSource source, std::string semantic,
                                 uint64_t minimum, uint64_t maximum, std::string unit) {
        ++snapshot->infrastructure_nodes_scanned;
        NodeCapability cap{};
        cap.path = path.string();
        cap.domain = domain;
        cap.source = source;
        cap.semantic_id = std::move(semantic);
        cap.unit = std::move(unit);
        cap.constraints = std::to_string(minimum) + "<=value<=" + std::to_string(maximum);
        cap.accepted_values = cap.constraints;
        cap.evidence = "Dynamically enumerated subsystem ABI with a bounded scalar contract; mutation remains baseline-gated";
        struct stat st{};
        if (::lstat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode) || S_ISLNK(st.st_mode)) {
            cap.rejection_reason = "node absent or not a regular ABI attribute";
            cap.rejection_code = NodeRejectionReason::NodeMissing;
            snapshot->node_capabilities.push_back(std::move(cap));
            return;
        }
        cap.state = NodeCapabilityState::Discovered;
        std::string value;
        uint64_t parsed = 0;
        if (!read_attribute(path, value, 256U)) {
            cap.rejection_reason = "bounded scalar read failed";
            cap.rejection_code = NodeRejectionReason::IoError;
            snapshot->node_capabilities.push_back(std::move(cap));
            return;
        }
        cap.current_value = trim(value);
        cap.state = NodeCapabilityState::Readable;
        if (!parse_u64(cap.current_value, parsed)) {
            cap.rejection_reason = "scalar ABI value is not an unsigned integer";
            cap.rejection_code = NodeRejectionReason::FormatMismatch;
            snapshot->node_capabilities.push_back(std::move(cap));
            return;
        }
        if (!writable_by_mode_and_process(path)) {
            cap.rejection_reason = "readable, but write permission was not established";
            cap.rejection_code = NodeRejectionReason::PermissionDenied;
            snapshot->node_capabilities.push_back(std::move(cap));
            return;
        }
        cap.state = NodeCapabilityState::AccessChecked;
        ++snapshot->infrastructure_nodes_writable;
        cap.state = NodeCapabilityState::SemanticsVerified;
        if (parsed < minimum || parsed > maximum) {
            cap.rejection_reason = "current scalar lies outside the verified subsystem contract";
            cap.rejection_code = NodeRejectionReason::ConstraintsUnverified;
            snapshot->node_capabilities.push_back(std::move(cap));
            return;
        }
        cap.state = NodeCapabilityState::ConstraintsVerified;
        auto& baseline = BaselineStore::instance();
        if (!baseline.capture(path.string())) {
            cap.rejection_reason = "OEM baseline capture failed during passive discovery";
            cap.rejection_code = NodeRejectionReason::BaselineCaptureFailed;
            snapshot->node_capabilities.push_back(std::move(cap));
            return;
        }
        const auto audit = baseline.get_audit_trail();
        const auto captured = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& node) {
            return node.path == path.string() && !node.is_modified && node.factory_value == cap.current_value;
        });
        const bool retained_owner = baseline.owns_retained_performance_value(
            path.string(), cap.current_value);
        if (captured == audit.end() && !retained_owner) {
            cap.rejection_reason = "OEM baseline is modified or no persisted CoreFlow retained-value ownership proof exists";
            cap.rejection_code = NodeRejectionReason::BaselineNotCaptured;
        } else {
            cap.state = NodeCapabilityState::MutationEligible;
            cap.rejection_code = NodeRejectionReason::None;
        }
        snapshot->node_capabilities.push_back(std::move(cap));
    };

    const fs::path vm_root = proc / "sys/vm";
    std::error_code vm_ec;
    for (fs::directory_iterator it(vm_root, fs::directory_options::skip_permission_denied, vm_ec), end;
         !vm_ec && it != end; it.increment(vm_ec)) {
        std::error_code type_ec;
        const auto status = it->symlink_status(type_ec);
        if (type_ec || fs::is_symlink(status) || !fs::is_regular_file(status)) continue;
        const std::string name = it->path().filename().string();
        if (name == "page-cluster")
            scan_scalar(it->path(), HardwareDomain::Memory, ProfileSource::Procfs, "vm.page_cluster", 0U, 8U, "pages");
        else if (name == "vfs_cache_pressure")
            scan_scalar(it->path(), HardwareDomain::Memory, ProfileSource::Procfs, "vm.vfs_cache_pressure", 50U, 200U, "ratio");
        else if (name == "dirty_background_ratio")
            scan_scalar(it->path(), HardwareDomain::Memory, ProfileSource::Procfs, "vm.dirty_background_ratio", 0U, 10U, "percent");
        else if (name == "dirty_ratio")
            scan_scalar(it->path(), HardwareDomain::Memory, ProfileSource::Procfs, "vm.dirty_ratio", 5U, 20U, "percent");
        else
            scan_observation(it->path(), HardwareDomain::Memory, ProfileSource::Procfs,
                "vm.tunable.unverified", "Dynamically enumerated VM sysctl; no portable mutation range contract");
    }

    const fs::path block_root = sys / "class/block";
    for (const auto& block : directory_entries(block_root)) {
        const fs::path queue = block / "queue";
        std::error_code queue_ec;
        for (fs::directory_iterator it(queue, fs::directory_options::skip_permission_denied, queue_ec), end;
             !queue_ec && it != end; it.increment(queue_ec)) {
            std::error_code type_ec;
            const auto status = it->symlink_status(type_ec);
            if (type_ec || fs::is_symlink(status) || !fs::is_regular_file(status)) continue;
            const std::string name = it->path().filename().string();
            if (name == "read_ahead_kb")
                scan_scalar(it->path(), HardwareDomain::Storage, ProfileSource::Sysfs,
                    "block.queue.read_ahead_kb", 32U, 512U, "KiB");
            else
                scan_observation(it->path(), HardwareDomain::Storage, ProfileSource::Sysfs,
                    "block.queue.tunable.unverified", "Dynamically enumerated block queue ABI; vendor range not assumed");
        }
    }

    if (read_attribute(proc / "meminfo", raw)) {
        snapshot->memory_telemetry_available = raw.find("MemTotal:") != std::string::npos &&
            raw.find("MemAvailable:") != std::string::npos;
    }
    // A single topology source resolves cgroup mounts, process-relative
    // hierarchy roots, and ABI-bearing groups. Discovery remains read-only.
    MountTopologyResolver mount_topology(proc_root_);
    const auto cgroup_mounts = mount_topology.discover_cgroup_mounts();
    const auto cgroup_groups = mount_topology.discover_cgroup_groups(4096U, 8U);
    const auto foreground_uclamp = mount_topology.resolve_foreground_uclamp_group();
    bool foreground_uclamp_baseline_ready = false;
    if (foreground_uclamp) {
        auto& baseline = BaselineStore::instance();
        const auto& min_path = foreground_uclamp->uclamp_min_path;
        const auto& max_path = foreground_uclamp->uclamp_max_path;
        const bool min_captured = !min_path.empty() && baseline.capture(min_path);
        const bool max_captured = !max_path.empty() && baseline.capture(max_path);
        const auto audit = baseline.get_audit_trail();
        const auto baseline_matches = [&](const std::string& path) {
            const auto node = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& item) {
                return item.path == path;
            });
            if (node != audit.end() && !node->is_modified &&
                node->factory_value == read_node_value(path)) return true;
            return baseline.owns_retained_performance_value(path, read_node_value(path));
        };
        foreground_uclamp_baseline_ready = min_captured && max_captured &&
            baseline_matches(min_path) && baseline_matches(max_path);
    }
    snapshot->cgroup_v2_available = std::any_of(cgroup_mounts.begin(), cgroup_mounts.end(),
        [](const CgroupMount& mount) { return mount.filesystem == CgroupFilesystem::V2; });
    snapshot->uclamp_interface_available = std::any_of(cgroup_groups.begin(), cgroup_groups.end(),
        [](const CgroupGroupCapability& group) { return group.has_uclamp_min && group.has_uclamp_max; });

    const auto append_cgroup_node = [&](const std::string& path, std::string semantic,
                                        std::string unit, HardwareDomain domain,
                                        bool verify_uclamp, bool verify_boolean,
                                        bool mutation_target = false) {
        NodeCapability cap{};
        cap.path = path;
        cap.domain = domain;
        cap.source = ProfileSource::KernelApi;
        cap.semantic_id = std::move(semantic);
        cap.unit = std::move(unit);
        cap.state = NodeCapabilityState::Discovered;
        cap.evidence = "Runtime mount topology plus Linux cgroup control-file ABI discovery; passive read-only probe";
        struct stat st{};
        if (::lstat(path.c_str(), &st) != 0) {
            cap.rejection_reason = "discovered ABI node disappeared before inspection";
            cap.rejection_code = NodeRejectionReason::NodeMissing;
            snapshot->node_capabilities.push_back(std::move(cap));
            return;
        }
        std::string value;
        if (!read_attribute(path, value, 1024U)) {
            cap.rejection_reason = "ABI node exists but bounded read failed";
            cap.rejection_code = (errno == EACCES && ::geteuid() == 0 && (st.st_mode & S_IWUSR) != 0)
                ? NodeRejectionReason::EaccesSelinux : NodeRejectionReason::IoError;
            snapshot->node_capabilities.push_back(std::move(cap));
            return;
        }
        cap.current_value = trim(value);
        cap.state = NodeCapabilityState::Readable;
        if (writable_by_mode_and_process(path)) {
            cap.state = NodeCapabilityState::AccessChecked;
            ++snapshot->infrastructure_nodes_writable;
        } else {
            cap.rejection_reason = "readable, but write access was not established; no write probe was attempted";
            cap.rejection_code = NodeRejectionReason::PermissionDenied;
        }
        if (verify_uclamp) {
            if (cap.state != NodeCapabilityState::AccessChecked) {
                snapshot->node_capabilities.push_back(std::move(cap));
                return;
            }
            const auto parsed = MountTopologyResolver::parse_uclamp_value(cap.current_value);
            if (!parsed) {
                cap.rejection_reason = "baseline node format is not a supported UClamp scalar";
                cap.rejection_code = NodeRejectionReason::FormatMismatch;
            } else {
                cap.state = NodeCapabilityState::SemanticsVerified;
                cap.constraints = parsed->encoding == UclampEncoding::Percent
                    ? "0.00<=percent<=100.00 or max" : "0<=capacity<=1024 or max";
                cap.state = NodeCapabilityState::ConstraintsVerified;
                cap.accepted_values = cap.constraints;
                if (mutation_target && foreground_uclamp_baseline_ready) {
                    cap.state = NodeCapabilityState::MutationEligible;
                    cap.rejection_reason.clear();
                    cap.rejection_code = NodeRejectionReason::None;
                } else {
                    cap.rejection_reason = mutation_target
                        ? "OEM UClamp baseline pair was not captured or retained ownership was not proven"
                        : "ABI is valid, but this group is not the uniquely resolved foreground hierarchy";
                    cap.rejection_code = NodeRejectionReason::BaselineNotCaptured;
                }
            }
        } else if (verify_boolean) {
            if (cap.state != NodeCapabilityState::AccessChecked) {
                snapshot->node_capabilities.push_back(std::move(cap));
                return;
            }
            const auto lower = cap.current_value;
            if (lower == "0" || lower == "1" || lower == "Enabled" || lower == "Disabled" ||
                lower == "enabled" || lower == "disabled") {
                cap.state = NodeCapabilityState::SemanticsVerified;
                cap.constraints = "boolean ABI values only";
                cap.accepted_values = "0|1|Enabled|Disabled";
                cap.state = NodeCapabilityState::ConstraintsVerified;
                cap.rejection_reason = "membership/control interface requires controller-specific ownership and task identity";
                cap.rejection_code = NodeRejectionReason::BaselineNotCaptured;
            } else {
                cap.rejection_reason = "unrecognized ABI value representation";
                cap.rejection_code = NodeRejectionReason::FormatMismatch;
            }
        }
        snapshot->node_capabilities.push_back(std::move(cap));
    };

    for (const auto& group : cgroup_groups) {
        const bool foreground_target = foreground_uclamp &&
            foreground_uclamp->mount.mount_id == group.mount.mount_id &&
            foreground_uclamp->absolute_path == group.absolute_path;
        if (group.has_uclamp_min)
            append_cgroup_node(group.uclamp_min_path, "cgroup.uclamp.min", "capacity", HardwareDomain::Cgroup, true, false, foreground_target);
        if (group.has_uclamp_max)
            append_cgroup_node(group.uclamp_max_path, "cgroup.uclamp.max", "capacity", HardwareDomain::Cgroup, true, false, foreground_target);
        if (!group.uclamp_latency_sensitive_path.empty())
            append_cgroup_node(group.uclamp_latency_sensitive_path, "cgroup.uclamp.latency_sensitive", "boolean", HardwareDomain::Cgroup, false, true);
        const std::string member_name = MountTopologyResolver::membership_name(group.membership);
        if (group.membership != CgroupMembershipInterface::None)
            append_cgroup_node(group.absolute_path + "/" + member_name,
                "cgroup.membership.thread", "tid", HardwareDomain::Cgroup, false, false);
        if (group.has_process_list)
            append_cgroup_node(group.absolute_path + "/cgroup.procs",
                "cgroup.membership.process", "pid", HardwareDomain::Cgroup, false, false);
    }
    // Merge a bounded dynamic parent/child inventory. Existing typed contracts
    // retain precedence; the generic inventory enriches their dependency links.
    KernelNodeInventory inventory(sys_root_, proc_root_);
    for (auto node : inventory.scan(512U)) {
        const auto existing = std::find_if(snapshot->node_capabilities.begin(),
            snapshot->node_capabilities.end(), [&](const NodeCapability& item) {
                return item.path == node.path;
            });
        if (existing == snapshot->node_capabilities.end()) {
            snapshot->node_capabilities.push_back(std::move(node));
        } else {
            if (existing->parent_policy_path.empty()) existing->parent_policy_path = std::move(node.parent_policy_path);
            if (existing->parent_policy_value.empty()) existing->parent_policy_value = std::move(node.parent_policy_value);
            if (existing->dependency_group.empty()) existing->dependency_group = std::move(node.dependency_group);
            existing->rediscover_after_parent_change = existing->rediscover_after_parent_change ||
                node.rediscover_after_parent_change;
        }
    }
    snapshot->fingerprint = make_fingerprint(*snapshot);
    return snapshot;
}

DeviceTopology DeviceProfiler::probe() {
    const auto current = profile();
    DeviceTopology topo{};
    topo.tier = DeviceTier::Entry; // Legacy presentation default only; no CPU-count tier inference.
    topo.total_cpus = static_cast<uint32_t>(current->present_cpus.size());
    topo.online_cpus_mask = join_cpu_ids(current->online_cpus);
    topo.kernel_version = current->kernel_release;
    topo.soc_name = !current->soc_model.empty() ? current->soc_model :
        (!current->device_model.empty() ? current->device_model : "Unknown SoC");
    topo.hardware_platform = current->hardware_platform;
    topo.has_cgroups_v2 = current->cgroup_v2_available;
    topo.has_uclamp = current->uclamp_interface_available;
    topo.has_surfaceflinger_hints = false;
    topo.gpu_type = current->gpu_domains.empty() ? "Unavailable" : current->gpu_domains.front().driver;
    for (const auto& policy : current->cpu_policies) {
        CpuCluster cluster{};
        cluster.cpufreq_policy_path = policy.policy_path;
        cluster.first_cpu = policy.related_cpus.empty() ? 0U : policy.related_cpus.front();
        cluster.last_cpu = policy.related_cpus.empty() ? 0U : policy.related_cpus.back();
        cluster.base_freq_khz = static_cast<uint32_t>(std::min<uint64_t>(policy.cpuinfo_min_freq_khz, UINT32_MAX));
        cluster.max_freq_khz = static_cast<uint32_t>(std::min<uint64_t>(policy.cpuinfo_max_freq_khz, UINT32_MAX));
        cluster.type = ClusterType::Medium;
        topo.clusters.push_back(std::move(cluster));
    }
    std::sort(topo.clusters.begin(), topo.clusters.end(), [](const auto& a, const auto& b) {
        return a.max_freq_khz < b.max_freq_khz;
    });
    if (!topo.clusters.empty()) {
        topo.clusters.front().type = ClusterType::Little;
        if (topo.clusters.size() > 1U) topo.clusters.back().type = ClusterType::BigPrime;
    }
    { std::lock_guard<std::mutex> lock(mutex_); cached_topology_ = topo; }
    return topo;
}
float DeviceProfiler::get_soc_temperature() const { return ThermalMonitor{}.hottest_celsius(); }
std::vector<uint32_t> DeviceProfiler::get_cluster_frequencies() const {
    const auto current = const_cast<DeviceProfiler*>(this)->profile();
    std::vector<uint32_t> frequencies;
    frequencies.reserve(current->cpu_policies.size());
    for (const auto& policy : current->cpu_policies) {
        const uint64_t value = policy.scaling_cur_freq_khz > 0 ?
            policy.scaling_cur_freq_khz : policy.scaling_max_freq_khz;
        frequencies.push_back(static_cast<uint32_t>(std::min<uint64_t>(value, UINT32_MAX)));
    }
    return frequencies;
}
} // namespace coreflow
