#include "coreflow/semantic/linux_sysfs_discovery_adapter.hpp"
#include "coreflow/mount_topology.hpp"

#include <array>
#include <cerrno>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <string_view>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

namespace coreflow::platform {
namespace {

constexpr std::size_t kPathCapacity = 768;
constexpr std::size_t kFileCapacity = 4096;
constexpr std::uint64_t kMaximumEvidenceAgeNs = 5'000'000'000ULL;
constexpr std::string_view kPolicyRoot = "devices/system/cpu/cpufreq/";
constexpr std::string_view kFrequencyNode = "/scaling_cur_freq";

[[nodiscard]] std::uint64_t monotonicNowNs() noexcept {
    timespec ts{};
    if (::clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return static_cast<std::uint64_t>(ts.tv_sec) * 1'000'000'000ULL +
           static_cast<std::uint64_t>(ts.tv_nsec);
}

[[nodiscard]] std::uint64_t hashBytes(std::uint64_t seed, std::string_view value) noexcept {
    auto hash = seed;
    for (const unsigned char byte : value) { hash ^= byte; hash *= 0x100000001b3ULL; }
    return hash == 0 ? 1 : hash;
}

[[nodiscard]] semantic::Identifier128 makeId(std::uint64_t a, std::uint64_t b) noexcept {
    semantic::Identifier128 id{};
    for (unsigned i = 0; i < 8; ++i) {
        id.bytes[i] = static_cast<std::uint8_t>((a >> (i * 8U)) & 0xffU);
        id.bytes[i + 8] = static_cast<std::uint8_t>((b >> (i * 8U)) & 0xffU);
    }
    if (!id.isValid()) id.bytes[0] = 1;
    return id;
}

[[nodiscard]] bool makePath(char* out, std::size_t capacity,
                            const char* root, const char* suffix) noexcept {
    if (root == nullptr || suffix == nullptr) return false;
    const int count = std::snprintf(out, capacity, "%s/%s", root, suffix);
    return count > 0 && static_cast<std::size_t>(count) < capacity;
}

[[nodiscard]] bool readFile(const char* path, char* out, std::size_t capacity,
                            std::size_t& length) noexcept {
    length = 0;
    const int fd = ::open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return false;
    bool ok = true;
    while (length + 1U < capacity) {
        const auto count = ::read(fd, out + length, capacity - length - 1U);
        if (count == 0) break;
        if (count < 0) { if (errno == EINTR) continue; ok = false; break; }
        length += static_cast<std::size_t>(count);
    }
    if (length + 1U == capacity) {
        char extra{};
        if (::read(fd, &extra, 1) != 0) ok = false;
    }
    ::close(fd);
    out[length] = '\0';
    return ok && length > 0U;
}

[[nodiscard]] std::string_view trimAscii(std::string_view value) noexcept {
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t' ||
                              value.front() == '\n' || value.front() == '\r')) value.remove_prefix(1);
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t' ||
                              value.back() == '\n' || value.back() == '\r')) value.remove_suffix(1);
    return value;
}

[[nodiscard]] bool parseUnsigned(std::string_view value, std::uint64_t& parsed) noexcept {
    value = trimAscii(value);
    if (value.empty()) return false;
    const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
    return result.ec == std::errc{} && result.ptr == value.data() + value.size();
}

[[nodiscard]] bool parsePositive(std::string_view value, std::uint64_t& parsed) noexcept {
    return parseUnsigned(value, parsed) && parsed > 0U;
}

[[nodiscard]] bool containsToken(std::string_view text, std::string_view token) noexcept {
    std::size_t pos = 0;
    while ((pos = text.find(token, pos)) != std::string_view::npos) {
        const bool left = pos == 0 || text[pos - 1] == ' ' || text[pos - 1] == '\n' || text[pos - 1] == '\t';
        const auto end = pos + token.size();
        const bool right = end == text.size() || text[end] == ' ' || text[end] == '\n' ||
                           text[end] == '\t' || text[end] == '\r';
        if (left && right) return true;
        pos = end;
    }
    return false;
}

[[nodiscard]] bool validatePsi(const char* path) noexcept {
    char contents[kFileCapacity]{};
    std::size_t length = 0;
    if (!readFile(path, contents, sizeof(contents), length)) return false;
    const std::string_view text(contents, length);
    if (!containsToken(text, "some") || text.find("avg10=") == std::string_view::npos ||
        text.find("total=") == std::string_view::npos) return false;
    std::uint64_t total = 0;
    const auto total_pos = text.find("total=");
    auto total_end = text.find_first_of(" \n\r", total_pos);
    if (total_end == std::string_view::npos) total_end = text.size();
    return parseUnsigned(text.substr(total_pos + 6U, total_end - total_pos - 6U), total);
}

[[nodiscard]] bool validUclamp(const std::string& path) noexcept {
    char contents[128]{};
    std::size_t length = 0;
    if (!readFile(path.c_str(), contents, sizeof(contents), length)) return false;
    return coreflow::MountTopologyResolver::parse_uclamp_value(
        std::string_view(contents, length)).has_value();
}

[[nodiscard]] bool parsePolicyLocator(std::string_view locator, std::string_view& policy) noexcept {
    constexpr std::string_view prefix = "sysfs:/";
    if (!locator.starts_with(prefix)) return false;
    locator.remove_prefix(prefix.size());
    if (!locator.starts_with(kPolicyRoot)) return false;
    locator.remove_prefix(kPolicyRoot.size());
    if (!locator.ends_with(kFrequencyNode)) return false;
    locator.remove_suffix(kFrequencyNode.size());
    if (locator.empty() || locator.find('/') != std::string_view::npos ||
        locator == "." || locator == "..") return false;
    for (const unsigned char ch : locator) {
        if (!(std::isalnum(ch) || ch == '_' || ch == '-' || ch == '.')) return false;
    }
    policy = locator;
    return true;
}

[[nodiscard]] bool makePolicyPath(char* out, std::size_t capacity, const char* sys_root,
                                  std::string_view policy, std::string_view attribute) noexcept {
    if (policy.empty() || policy.find('/') != std::string_view::npos || attribute.empty() ||
        attribute.find("..") != std::string_view::npos) return false;
    const int written = std::snprintf(out, capacity, "%s/%.*s%.*s/%.*s", sys_root,
        static_cast<int>(kPolicyRoot.size()), kPolicyRoot.data(),
        static_cast<int>(policy.size()), policy.data(),
        static_cast<int>(attribute.size()), attribute.data());
    return written > 0 && static_cast<std::size_t>(written) < capacity;
}

[[nodiscard]] bool readPolicyAttribute(const char* root, std::string_view policy,
                                       std::string_view attribute, char* output,
                                       std::size_t capacity, std::size_t& length) noexcept {
    char path[kPathCapacity]{};
    return makePolicyPath(path, sizeof(path), root, policy, attribute) &&
           readFile(path, output, capacity, length);
}

[[nodiscard]] std::vector<coreflow::CgroupGroupCapability> discoverCpuUclampGroups(
    const char* proc_root) {
    coreflow::MountTopologyResolver resolver(proc_root == nullptr ? "/proc" : proc_root);
    std::vector<coreflow::CgroupGroupCapability> result;
    for (auto& group : resolver.discover_cgroup_groups(2048U, 8U)) {
        if (group.has_uclamp_min && group.has_uclamp_max) result.push_back(std::move(group));
    }
    return result;
}

} // namespace

LinuxSysfsDiscoveryAdapter::LinuxSysfsDiscoveryAdapter(
    std::string_view sysfs_root, std::string_view procfs_root) noexcept {
    if (sysfs_root.empty() || procfs_root.empty() ||
        sysfs_root.size() >= sysfs_root_.size() || procfs_root.size() >= procfs_root_.size()) return;
    std::memcpy(sysfs_root_.data(), sysfs_root.data(), sysfs_root.size());
    std::memcpy(procfs_root_.data(), procfs_root.data(), procfs_root.size());
    roots_valid_ = true;
}

std::vector<RawDiscoveredEndpoint> LinuxSysfsDiscoveryAdapter::discover() noexcept {
    std::vector<RawDiscoveredEndpoint> endpoints{};
    if (!roots_valid_) return endpoints;
    char root[kPathCapacity]{};
    if (!makePath(root, sizeof(root), sysfs_root_.data(), kPolicyRoot.data())) return endpoints;
    DIR* directory = ::opendir(root);
    if (directory == nullptr) return endpoints;
    const auto generation = monotonicNowNs();
    if (generation == 0U) { ::closedir(directory); return endpoints; }
    while (dirent* entry = ::readdir(directory)) {
        if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0) continue;
        struct stat st{};
        if (::fstatat(::dirfd(directory), entry->d_name, &st, AT_SYMLINK_NOFOLLOW) != 0 ||
            !S_ISDIR(st.st_mode) || S_ISLNK(st.st_mode)) continue;
        const std::string_view policy(entry->d_name);
        char driver[kFileCapacity]{}, current[128]{};
        std::size_t driver_length = 0, current_length = 0;
        if (!readPolicyAttribute(sysfs_root_.data(), policy, "scaling_driver",
                                 driver, sizeof(driver), driver_length) ||
            trimAscii(std::string_view(driver, driver_length)).empty() ||
            !readPolicyAttribute(sysfs_root_.data(), policy, "scaling_cur_freq",
                                 current, sizeof(current), current_length)) continue;
        std::uint64_t current_khz = 0;
        if (!parsePositive(std::string_view(current, current_length), current_khz)) continue;
        std::string locator = "sysfs:/" + std::string(kPolicyRoot) + std::string(policy) +
                              std::string(kFrequencyNode);
        const auto first = hashBytes(0xcbf29ce484222325ULL, locator);
        const auto second = hashBytes(first, trimAscii(std::string_view(driver, driver_length)));
        RawDiscoveredEndpoint endpoint{};
        endpoint.probe_id = makeId(first, second);
        endpoint.opaque_locator = std::move(locator);
        endpoint.raw_access = semantic::AccessCapability::Read;
        endpoint.observed_generation = generation;
        endpoints.push_back(std::move(endpoint));
    }
    ::closedir(directory);
    return endpoints;
}

std::optional<std::string> LinuxSysfsDiscoveryAdapter::readPassive(
    const RawDiscoveredEndpoint& endpoint) noexcept {
    std::string_view policy;
    if (!roots_valid_ || !endpoint.probe_id.isValid() || endpoint.observed_generation == 0 ||
        !parsePolicyLocator(endpoint.opaque_locator, policy) ||
        !semantic::hasAccess(endpoint.raw_access, semantic::AccessCapability::Read)) return std::nullopt;
    char value[128]{};
    std::size_t length = 0;
    if (!readPolicyAttribute(sysfs_root_.data(), policy, "scaling_cur_freq", value, sizeof(value), length))
        return std::nullopt;
    const auto normalized = trimAscii(std::string_view(value, length));
    std::uint64_t parsed = 0;
    if (!parsePositive(normalized, parsed)) return std::nullopt;
    return std::string(normalized);
}

NumericFormatStream LinuxSysfsDiscoveryAdapter::readNumericFormat(
    const RawDiscoveredEndpoint& endpoint) noexcept {
    std::string_view policy;
    if (!roots_valid_ || !endpoint.probe_id.isValid() || endpoint.observed_generation == 0 ||
        !parsePolicyLocator(endpoint.opaque_locator, policy) ||
        !semantic::hasAccess(endpoint.raw_access, semantic::AccessCapability::Read)) return {};
    char minimum[128]{}, maximum[128]{};
    std::size_t min_length = 0, max_length = 0;
    if (!readPolicyAttribute(sysfs_root_.data(), policy, "cpuinfo_min_freq", minimum, sizeof(minimum), min_length) ||
        !readPolicyAttribute(sysfs_root_.data(), policy, "cpuinfo_max_freq", maximum, sizeof(maximum), max_length)) return {};
    std::uint64_t min_value = 0, max_value = 0;
    if (!parsePositive(std::string_view(minimum, min_length), min_value) ||
        !parsePositive(std::string_view(maximum, max_length), max_value) || min_value >= max_value ||
        max_value > static_cast<std::uint64_t>(INT64_MAX)) return {};
    NumericFormatStream result{};
    result.has_minimum = true; result.has_maximum = true;
    result.minimum = static_cast<std::int64_t>(min_value);
    result.maximum = static_cast<std::int64_t>(max_value);
    return result;
}

std::size_t LinuxSysfsDiscoveryAdapter::collectDomainRelationshipEvidence(
    const RawDiscoveredEndpoint& endpoint,
    std::span<semantic::DomainRelationshipEvidence> output) noexcept {
    using namespace semantic;
    std::string_view policy;
    if (!roots_valid_ || output.size() < 3U || !endpoint.probe_id.isValid() ||
        endpoint.observed_generation == 0U || !parsePolicyLocator(endpoint.opaque_locator, policy) ||
        !hasAccess(endpoint.raw_access, AccessCapability::Read)) return 0U;

    const auto started = monotonicNowNs();
    if (started == 0U) return 0U;
    char driver[kFileCapacity]{}, frequencies[kFileCapacity]{}, minimum[kFileCapacity]{}, maximum[kFileCapacity]{};
    std::size_t driver_length = 0, frequencies_length = 0, min_length = 0, max_length = 0;
    if (!readPolicyAttribute(sysfs_root_.data(), policy, "scaling_driver", driver, sizeof(driver), driver_length) ||
        !readPolicyAttribute(sysfs_root_.data(), policy, "scaling_available_frequencies",
                             frequencies, sizeof(frequencies), frequencies_length) ||
        !readPolicyAttribute(sysfs_root_.data(), policy, "cpuinfo_min_freq", minimum, sizeof(minimum), min_length) ||
        !readPolicyAttribute(sysfs_root_.data(), policy, "cpuinfo_max_freq", maximum, sizeof(maximum), max_length)) return 0U;

    std::uint64_t min_value = 0, max_value = 0, frequency_value = 0;
    if (!parsePositive(std::string_view(minimum, min_length), min_value) ||
        !parsePositive(std::string_view(maximum, max_length), max_value) || min_value >= max_value) return 0U;
    std::string_view frequency_list(frequencies, frequencies_length);
    bool frequency_valid = false;
    while (!frequency_list.empty()) {
        while (!frequency_list.empty() && (frequency_list.front() == ' ' || frequency_list.front() == '\t' ||
               frequency_list.front() == '\r' || frequency_list.front() == '\n')) frequency_list.remove_prefix(1U);
        if (frequency_list.empty()) break;
        const auto split = frequency_list.find_first_of(" \t\r\n");
        const auto token = frequency_list.substr(0, split);
        if (!parsePositive(token, frequency_value) || frequency_value < min_value || frequency_value > max_value) return 0U;
        frequency_valid = true;
        if (split == std::string_view::npos) break;
        frequency_list.remove_prefix(split + 1U);
    }
    const auto driver_name = trimAscii(std::string_view(driver, driver_length));
    if (!frequency_valid || driver_name.empty() || driver_name.find_first_of(" \t\r\n") != std::string_view::npos)
        return 0U;

    bool cpu_cgroup_valid = false;
    const auto groups = discoverCpuUclampGroups(procfs_root_.data());
    for (const auto& group : groups) {
        char cpu_stat[kFileCapacity]{};
        std::size_t cpu_stat_length = 0;
        const std::string stat_path = group.absolute_path + "/cpu.stat";
        const std::string pressure_path = group.absolute_path + "/cpu.pressure";
        const bool stat_ok = readFile(stat_path.c_str(), cpu_stat, sizeof(cpu_stat), cpu_stat_length) &&
            containsToken(std::string_view(cpu_stat, cpu_stat_length), "usage_usec");
        if (group.mount.filesystem == CgroupFilesystem::V2) {
            cpu_cgroup_valid = stat_ok && validUclamp(group.uclamp_min_path) &&
                validUclamp(group.uclamp_max_path) && validatePsi(pressure_path.c_str());
        } else {
            char tasks[kFileCapacity]{}, procs[kFileCapacity]{}, pressure[kFileCapacity]{};
            std::size_t tasks_length = 0, procs_length = 0, pressure_length = 0;
            const bool tasks_ok = readFile((group.absolute_path + "/tasks").c_str(), tasks, sizeof(tasks), tasks_length) ||
                readFile((group.absolute_path + "/cgroup.threads").c_str(), tasks, sizeof(tasks), tasks_length);
            const bool procs_ok = readFile((group.absolute_path + "/cgroup.procs").c_str(), procs, sizeof(procs), procs_length);
            const bool psi_ok = makePath(pressure, sizeof(pressure), procfs_root_.data(), "pressure/cpu") &&
                validatePsi(pressure);
            cpu_cgroup_valid = validUclamp(group.uclamp_min_path) && validUclamp(group.uclamp_max_path) &&
                tasks_ok && procs_ok && psi_ok;
            (void)pressure_length;
        }
        if (cpu_cgroup_valid) break;
    }
    if (!cpu_cgroup_valid) return 0U;

    const auto observed = monotonicNowNs();
    if (observed == 0U || observed < started || observed - started > kMaximumEvidenceAgeNs) return 0U;
    const CapabilityId capability{endpoint.probe_id};
    const auto relation_hash = hashBytes(0xcbf29ce484222325ULL, std::string_view("coreflow.cpu.dvfs-cgroup-v2"));
    const Identifier128 relationship = makeId(
        hashBytes(relation_hash, std::string_view(reinterpret_cast<const char*>(endpoint.probe_id.bytes.data()),
                                                   endpoint.probe_id.bytes.size())), relation_hash);
    constexpr std::array<DomainEvidenceKind, 3> kinds{
        DomainEvidenceKind::SubsystemAssociation, DomainEvidenceKind::DriverAbi,
        DomainEvidenceKind::PsiResponseCorrelation};
    for (std::size_t i = 0; i < kinds.size(); ++i) {
        const auto ordinal = static_cast<std::uint64_t>(i + 1U);
        output[i] = DomainRelationshipEvidence{
            kinds[i], FunctionalDomain::Cpu,
            EvidenceId{makeId(relation_hash ^ ordinal,
                hashBytes(ordinal, std::string_view(reinterpret_cast<const char*>(endpoint.probe_id.bytes.data()), 16)))},
            ProvenanceId{makeId(relation_hash ^ (ordinal << 8U), hashBytes(ordinal + 17U, driver_name))},
            relationship, endpoint.observed_generation, observed, true, capability};
    }
    return 3U;
}

} // namespace coreflow::platform
