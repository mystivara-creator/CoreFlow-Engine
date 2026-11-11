#include "coreflow/semantic/linux_sysfs_discovery_adapter.hpp"

#include <array>
#include <cerrno>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string_view>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

namespace coreflow::platform {
namespace {

constexpr std::size_t kPathCapacity = 768;
constexpr std::size_t kFileCapacity = 4096;
constexpr std::uint64_t kMaximumEvidenceAgeNs = 5'000'000'000ULL;

[[nodiscard]] std::uint64_t monotonicNowNs() noexcept {
    timespec ts{};
    if (::clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return static_cast<std::uint64_t>(ts.tv_sec) * 1'000'000'000ULL +
           static_cast<std::uint64_t>(ts.tv_nsec);
}

[[nodiscard]] std::uint64_t hashBytes(std::uint64_t seed, std::string_view value) noexcept {
    auto hash = seed;
    for (const unsigned char byte : value) {
        hash ^= byte;
        hash *= 0x100000001b3ULL;
    }
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
    const int fd = ::open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) return false;
    bool ok = true;
    while (length + 1 < capacity) {
        const auto count = ::read(fd, out + length, capacity - length - 1);
        if (count == 0) break;
        if (count < 0) {
            if (errno == EINTR) continue;
            ok = false;
            break;
        }
        length += static_cast<std::size_t>(count);
    }
    if (length + 1 == capacity) {
        char extra{};
        const auto count = ::read(fd, &extra, 1);
        if (count != 0) ok = false;
    }
    ::close(fd);
    out[length] = '\0';
    return ok && length > 0;
}

[[nodiscard]] bool parseUnsigned(std::string_view value, std::uint64_t& parsed) noexcept {
    while (!value.empty() && (value.front() == ' ' || value.front() == '\\t' ||
                              value.front() == '\\n' || value.front() == '\\r'))
        value.remove_prefix(1);
    while (!value.empty() && (value.back() == ' ' || value.back() == '\\t' ||
                              value.back() == '\\n' || value.back() == '\\r'))
        value.remove_suffix(1);
    if (value.empty()) return false;
    const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
    return result.ec == std::errc{} && result.ptr == value.data() + value.size();
}

[[nodiscard]] bool parsePositive(std::string_view value, std::uint64_t& parsed) noexcept {
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t' ||
                              value.front() == '\n' || value.front() == '\r'))
        value.remove_prefix(1);
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t' ||
                              value.back() == '\n' || value.back() == '\r'))
        value.remove_suffix(1);
    if (value.empty()) return false;
    const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
    return result.ec == std::errc{} && result.ptr == value.data() + value.size() && parsed > 0;
}

[[nodiscard]] bool containsToken(std::string_view text, std::string_view token) noexcept {
    std::size_t pos = 0;
    while ((pos = text.find(token, pos)) != std::string_view::npos) {
        const bool left = pos == 0 || text[pos - 1] == ' ' || text[pos - 1] == '\n' ||
                          text[pos - 1] == '\t';
        const auto end = pos + token.size();
        const bool right = end == text.size() || text[end] == ' ' || text[end] == '\n' ||
                           text[end] == '\t' || text[end] == '\r';
        if (left && right) return true;
        pos = end;
    }
    return false;
}

[[nodiscard]] bool parseCgroupPath(const char* proc_root, char* out,
                                   std::size_t capacity) noexcept {
    char path[kPathCapacity]{};
    char contents[kFileCapacity]{};
    std::size_t length = 0;
    if (!makePath(path, sizeof(path), proc_root, "self/cgroup") ||
        !readFile(path, contents, sizeof(contents), length)) return false;
    std::string_view text(contents, length);
    constexpr std::string_view marker = "0::";
    const auto pos = text.find(marker);
    if (pos == std::string_view::npos || (pos != 0 && text[pos - 1] != '\n')) return false;
    auto end = text.find('\n', pos);
    if (end == std::string_view::npos) end = text.size();
    auto value = text.substr(pos + marker.size(), end - pos - marker.size());
    if (value.empty() || value.front() != '/' || value.find("..") != std::string_view::npos ||
        value.find('\\') != std::string_view::npos) return false;
    if (value.size() >= capacity) return false;
    std::memcpy(out, value.data(), value.size());
    out[value.size()] = '\0';
    return true;
}

[[nodiscard]] bool findCgroup2Mount(const char* proc_root, char* out,
                                    std::size_t capacity) noexcept {
    char path[kPathCapacity]{};
    char contents[kFileCapacity]{};
    std::size_t length = 0;
    if (!makePath(path, sizeof(path), proc_root, "self/mountinfo") ||
        !readFile(path, contents, sizeof(contents), length)) return false;
    std::string_view text(contents, length);
    std::size_t cursor = 0;
    unsigned matches = 0;
    while (cursor < text.size()) {
        auto end = text.find('\n', cursor);
        if (end == std::string_view::npos) end = text.size();
        auto line = text.substr(cursor, end - cursor);
        cursor = end + 1;
        const auto separator = line.find(" - ");
        if (separator == std::string_view::npos) return false;
        auto right = line.substr(separator + 3);
        if (right.starts_with("cgroup2 ")) {
            ++matches;
            // mountinfo fields: id parent major:minor root mountpoint ...
            auto left = line.substr(0, separator);
            std::size_t field = 0, start = 0;
            std::string_view mountpoint{};
            while (start < left.size()) {
                while (start < left.size() && left[start] == ' ') ++start;
                auto stop = left.find(' ', start);
                if (stop == std::string_view::npos) stop = left.size();
                if (field == 4) mountpoint = left.substr(start, stop - start);
                ++field;
                start = stop + 1;
            }
            if (field < 6 || mountpoint.empty() || mountpoint.front() != '/' ||
                mountpoint.find("..") != std::string_view::npos ||
                mountpoint.size() >= capacity) return false;
            std::memcpy(out, mountpoint.data(), mountpoint.size());
            out[mountpoint.size()] = '\0';
        }
    }
    return matches == 1;
}

[[nodiscard]] bool validatePsi(const char* path) noexcept {
    char contents[kFileCapacity]{};
    std::size_t length = 0;
    if (!readFile(path, contents, sizeof(contents), length)) return false;
    const std::string_view text(contents, length);
    if (!containsToken(text, "some") || text.find("avg10=") == std::string_view::npos ||
        text.find("total=") == std::string_view::npos) return false;
    // Reject malformed/overflowed PSI records rather than treating presence as proof.
    std::uint64_t total = 0;
    const auto total_pos = text.find("total=");
    auto total_end = text.find_first_of(" \n\r", total_pos);
    if (total_end == std::string_view::npos) total_end = text.size();
    return parseUnsigned(text.substr(total_pos + 6, total_end - total_pos - 6), total);
}

[[nodiscard]] bool validUclamp(const char* path) noexcept {
    char contents[128]{};
    std::size_t length = 0;
    if (!readFile(path, contents, sizeof(contents), length)) return false;
    std::uint64_t value = 0;
    return parseUnsigned(std::string_view(contents, length), value) && value <= 1024;
}

} // namespace

LinuxSysfsDiscoveryAdapter::LinuxSysfsDiscoveryAdapter(
    std::string_view sysfs_root, std::string_view procfs_root) noexcept {
    if (sysfs_root.empty() || procfs_root.empty() ||
        sysfs_root.size() >= sysfs_root_.size() ||
        procfs_root.size() >= procfs_root_.size()) return;
    std::memcpy(sysfs_root_.data(), sysfs_root.data(), sysfs_root.size());
    std::memcpy(procfs_root_.data(), procfs_root.data(), procfs_root.size());
    roots_valid_ = true;
}

std::vector<RawDiscoveredEndpoint> LinuxSysfsDiscoveryAdapter::discover() noexcept {
    return {};
}

std::optional<std::string> LinuxSysfsDiscoveryAdapter::readPassive(
    const RawDiscoveredEndpoint&) noexcept {
    return std::nullopt;
}

NumericFormatStream LinuxSysfsDiscoveryAdapter::readNumericFormat(
    const RawDiscoveredEndpoint&) noexcept {
    return {};
}

std::size_t LinuxSysfsDiscoveryAdapter::collectDomainRelationshipEvidence(
    const RawDiscoveredEndpoint& endpoint,
    std::span<semantic::DomainRelationshipEvidence> output) noexcept {
    using namespace semantic;
    if (!roots_valid_ || output.size() < 3 || !endpoint.probe_id.isValid() ||
        endpoint.observed_generation == 0) return 0;

    const auto started = monotonicNowNs();
    if (started == 0) return 0;

    char path[kPathCapacity]{};
    char driver[kFileCapacity]{};
    std::size_t driver_length = 0;
    char frequencies[kFileCapacity]{};
    std::size_t frequencies_length = 0;
    char minimum[kFileCapacity]{}, maximum[kFileCapacity]{};
    std::size_t min_length = 0, max_length = 0;

    const bool cpu_paths =
        makePath(path, sizeof(path), sysfs_root_.data(),
                 "devices/system/cpu/cpufreq/policy0/scaling_driver") &&
        readFile(path, driver, sizeof(driver), driver_length) &&
        makePath(path, sizeof(path), sysfs_root_.data(),
                 "devices/system/cpu/cpufreq/policy0/scaling_available_frequencies") &&
        readFile(path, frequencies, sizeof(frequencies), frequencies_length) &&
        makePath(path, sizeof(path), sysfs_root_.data(),
                 "devices/system/cpu/cpufreq/policy0/cpuinfo_min_freq") &&
        readFile(path, minimum, sizeof(minimum), min_length) &&
        makePath(path, sizeof(path), sysfs_root_.data(),
                 "devices/system/cpu/cpufreq/policy0/cpuinfo_max_freq") &&
        readFile(path, maximum, sizeof(maximum), max_length);
    if (!cpu_paths) return 0;

    std::uint64_t min_value = 0, max_value = 0, frequency_value = 0;
    if (!parsePositive(std::string_view(minimum, min_length), min_value) ||
        !parsePositive(std::string_view(maximum, max_length), max_value) ||
        min_value >= max_value) return 0;
    std::string_view frequency_list(frequencies, frequencies_length);
    bool frequency_valid = false;
    while (!frequency_list.empty()) {
        while (!frequency_list.empty() && (frequency_list.front() == ' ' ||
               frequency_list.front() == '\\t' || frequency_list.front() == '\\r' ||
               frequency_list.front() == '\\n')) frequency_list.remove_prefix(1);
        if (frequency_list.empty()) break;
        const auto split = frequency_list.find_first_of(" \\t\\r\\n");
        const auto token = frequency_list.substr(0, split);
        if (!parsePositive(token, frequency_value) ||
            frequency_value < min_value || frequency_value > max_value) return 0;
        frequency_valid = true;
        if (split == std::string_view::npos) break;
        frequency_list.remove_prefix(split + 1);
    }
    const std::string_view driver_name(driver, driver_length);
    if (!frequency_valid || driver_name.empty() ||
        driver_name.find_first_of(" \\t\\r\\n") != std::string_view::npos) return 0;

    char cgroup_path[kPathCapacity]{}, mountpoint[kPathCapacity]{};
    if (!parseCgroupPath(procfs_root_.data(), cgroup_path, sizeof(cgroup_path)) ||
        !findCgroup2Mount(procfs_root_.data(), mountpoint, sizeof(mountpoint))) return 0;

    char cgroup_root[kPathCapacity]{}, cgroup_file[kPathCapacity]{};
    int written = std::snprintf(cgroup_root, sizeof(cgroup_root), "%s%s",
                                mountpoint, cgroup_path);
    if (written <= 0 || static_cast<std::size_t>(written) >= sizeof(cgroup_root)) return 0;

    written = std::snprintf(cgroup_file, sizeof(cgroup_file), "%s/cpu.uclamp.min", cgroup_root);
    if (written <= 0 || static_cast<std::size_t>(written) >= sizeof(cgroup_file) ||
        !validUclamp(cgroup_file)) return 0;
    written = std::snprintf(cgroup_file, sizeof(cgroup_file), "%s/cpu.uclamp.max", cgroup_root);
    if (written <= 0 || static_cast<std::size_t>(written) >= sizeof(cgroup_file) ||
        !validUclamp(cgroup_file)) return 0;
    written = std::snprintf(cgroup_file, sizeof(cgroup_file), "%s/cpu.stat", cgroup_root);
    char cpu_stat[kFileCapacity]{};
    std::size_t cpu_stat_length = 0;
    if (written <= 0 || static_cast<std::size_t>(written) >= sizeof(cgroup_file) ||
        !readFile(cgroup_file, cpu_stat, sizeof(cpu_stat), cpu_stat_length) ||
        !containsToken(std::string_view(cpu_stat, cpu_stat_length), "usage_usec")) return 0;

    written = std::snprintf(cgroup_file, sizeof(cgroup_file), "%s/cpu.pressure", cgroup_root);
    if (written <= 0 || static_cast<std::size_t>(written) >= sizeof(cgroup_file) ||
        !validatePsi(cgroup_file)) return 0;

    const auto observed = monotonicNowNs();
    if (observed == 0 || observed < started || observed - started > kMaximumEvidenceAgeNs)
        return 0;

    const CapabilityId capability{endpoint.probe_id};
    const auto relation_hash = hashBytes(0xcbf29ce484222325ULL,
                                         std::string_view("coreflow.cpu.dvfs-cgroup-v2"));
    const Identifier128 relationship = makeId(
        hashBytes(relation_hash, std::string_view(
            reinterpret_cast<const char*>(endpoint.probe_id.bytes.data()),
            endpoint.probe_id.bytes.size())), relation_hash);
    constexpr std::array<DomainEvidenceKind, 3> kinds{
        DomainEvidenceKind::SubsystemAssociation,
        DomainEvidenceKind::DriverAbi,
        DomainEvidenceKind::PsiResponseCorrelation};
    for (std::size_t i = 0; i < kinds.size(); ++i) {
        const auto ordinal = static_cast<std::uint64_t>(i + 1);
        output[i] = DomainRelationshipEvidence{
            kinds[i], FunctionalDomain::Cpu,
            EvidenceId{makeId(relation_hash ^ ordinal,
                              hashBytes(ordinal, std::string_view(
                                  reinterpret_cast<const char*>(endpoint.probe_id.bytes.data()), 16)))},
            ProvenanceId{makeId(relation_hash ^ (ordinal << 8U),
                                hashBytes(ordinal + 17U, std::string_view(driver, driver_length)))},
            relationship, endpoint.observed_generation, observed, true, capability};
    }
    return 3;
}

} // namespace coreflow::platform
