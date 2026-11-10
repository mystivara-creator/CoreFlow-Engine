#include "coreflow/frequency_target_planner.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cerrno>
#include <fcntl.h>
#include <limits>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace coreflow {
namespace {

constexpr std::size_t kMaxSysfsBytes = 32U * 1024U;
constexpr std::size_t kMaxOperatingPoints = 512U;
constexpr double kMinimumCapacityScale = 0.50;
constexpr double kMaximumCapacityScale = 1.25;

struct TextBuffer final {
    std::array<char, kMaxSysfsBytes + 1U> bytes{};
    std::size_t size{0};
};

bool read_sysfs_text(const std::string& path, TextBuffer& output) noexcept {
    output = {};
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) {
        return false;
    }

    struct stat metadata {};
    if (::fstat(fd, &metadata) != 0 || !S_ISREG(metadata.st_mode) ||
        metadata.st_size > static_cast<off_t>(kMaxSysfsBytes)) {
        (void)::close(fd);
        return false;
    }

    while (output.size < kMaxSysfsBytes) {
        const ssize_t count = ::read(
            fd,
            output.bytes.data() + output.size,
            kMaxSysfsBytes - output.size);
        if (count > 0) {
            output.size += static_cast<std::size_t>(count);
            continue;
        }
        if (count == 0) {
            break;
        }
        if (errno == EINTR) {
            continue;
        }
        (void)::close(fd);
        return false;
    }

    if (output.size == kMaxSysfsBytes) {
        char extra = 0;
        const ssize_t count = ::read(fd, &extra, 1U);
        if (count > 0) {
            (void)::close(fd);
            return false;
        }
        if (count < 0 && errno != EINTR) {
            (void)::close(fd);
            return false;
        }
    }

    (void)::close(fd);
    output.bytes[output.size] = '\0';
    return output.size != 0U;
}

bool parse_u64_allow_zero(std::string_view token, uint64_t& value) noexcept {
    while (!token.empty() &&
           (token.front() == ' ' || token.front() == '\\t' ||
            token.front() == '\\r' || token.front() == '\\n')) {
        token.remove_prefix(1U);
    }
    while (!token.empty() &&
           (token.back() == ' ' || token.back() == '\\t' ||
            token.back() == '\\r' || token.back() == '\\n')) {
        token.remove_suffix(1U);
    }
    if (token.empty()) {
        return false;
    }
    uint64_t parsed = 0U;
    const auto result = std::from_chars(
        token.data(), token.data() + token.size(), parsed, 10);
    if (result.ec != std::errc{} || result.ptr != token.data() + token.size()) {
        return false;
    }
    value = parsed;
    return true;
}

bool parse_u64(std::string_view token, uint64_t& value) noexcept {
    while (!token.empty() &&
           (token.front() == ' ' || token.front() == '\t' ||
            token.front() == '\r' || token.front() == '\n')) {
        token.remove_prefix(1U);
    }
    while (!token.empty() &&
           (token.back() == ' ' || token.back() == '\t' ||
            token.back() == '\r' || token.back() == '\n')) {
        token.remove_suffix(1U);
    }
    if (token.empty()) {
        return false;
    }

    uint64_t parsed = 0U;
    const auto result = std::from_chars(
        token.data(), token.data() + token.size(), parsed, 10);
    if (result.ec != std::errc{} || result.ptr != token.data() + token.size() ||
        parsed == 0U) {
        return false;
    }
    value = parsed;
    return true;
}

bool read_khz_value(const std::string& path, uint64_t& hz) noexcept {
    TextBuffer text{};
    if (!read_sysfs_text(path, text)) {
        return false;
    }
    const std::string_view view(text.bytes.data(), text.size);
    uint64_t khz = 0U;
    if (!parse_u64(view, khz) ||
        khz > std::numeric_limits<uint64_t>::max() / 1000U) {
        return false;
    }
    hz = khz * 1000U;
    return true;
}

bool append_khz(std::string_view token, std::vector<uint64_t>& output) noexcept {
    uint64_t khz = 0U;
    if (!parse_u64(token, khz) ||
        khz > std::numeric_limits<uint64_t>::max() / 1000U ||
        output.size() >= kMaxOperatingPoints) {
        return false;
    }
    output.push_back(khz * 1000U);
    return true;
}

bool parse_available_frequencies(
    const TextBuffer& text,
    std::vector<uint64_t>& output) noexcept {
    output.clear();
    const std::string_view input(text.bytes.data(), text.size);
    std::size_t offset = 0U;

    while (offset < input.size()) {
        while (offset < input.size() &&
               (input[offset] == ' ' || input[offset] == '\t' ||
                input[offset] == '\r' || input[offset] == '\n')) {
            ++offset;
        }
        if (offset >= input.size()) {
            break;
        }

        const std::size_t start = offset;
        while (offset < input.size() &&
               input[offset] != ' ' && input[offset] != '\t' &&
               input[offset] != '\r' && input[offset] != '\n') {
            ++offset;
        }
        if (!append_khz(input.substr(start, offset - start), output)) {
            output.clear();
            return false;
        }
    }

    if (output.empty()) {
        return false;
    }
    std::sort(output.begin(), output.end());
    output.erase(std::unique(output.begin(), output.end()), output.end());
    return !output.empty();
}

bool parse_time_in_state(
    const TextBuffer& text,
    std::vector<uint64_t>& output) noexcept {
    output.clear();
    const std::string_view input(text.bytes.data(), text.size);
    std::size_t offset = 0U;

    while (offset < input.size()) {
        const std::size_t line_start = offset;
        const std::size_t line_end = input.find('\n', offset);
        offset = line_end == std::string_view::npos ? input.size() : line_end + 1U;
        std::string_view line = input.substr(
            line_start,
            (line_end == std::string_view::npos ? input.size() : line_end) - line_start);

        while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) {
            line.remove_prefix(1U);
        }
        if (line.empty()) {
            continue;
        }

        const std::size_t separator = line.find_first_of(" \t");
        if (separator == std::string_view::npos) {
            output.clear();
            return false;
        }

        uint64_t time_in_state = 0U;
        if (!append_khz(line.substr(0U, separator), output) ||
            !parse_u64_allow_zero(line.substr(separator + 1U), time_in_state)) {
            output.clear();
            return false;
        }
    }

    if (output.empty()) {
        return false;
    }
    std::sort(output.begin(), output.end());
    output.erase(std::unique(output.begin(), output.end()), output.end());
    return !output.empty();
}

bool read_frequency_table(
    const std::string& policy_path,
    std::vector<uint64_t>& frequencies) noexcept {
    TextBuffer text{};
    if (read_sysfs_text(policy_path + "/scaling_available_frequencies", text) &&
        parse_available_frequencies(text, frequencies)) {
        return true;
    }

    if (read_sysfs_text(policy_path + "/stats/time_in_state", text) &&
        parse_time_in_state(text, frequencies)) {
        return true;
    }
    return false;
}

} // namespace

std::optional<FrequencyCapabilitySnapshot>
FrequencyOppDiscovery::discover_cpu_policy(
    const FrequencyDiscoveryRequest& request) const noexcept {
    if (sys_root_.empty() || sys_root_.size() > 1024U || request.policy_index > 255U) {
        return std::nullopt;
    }

    const std::string policy_path =
        sys_root_ + "/devices/system/cpu/cpufreq/policy" +
        std::to_string(request.policy_index);

    uint64_t hardware_min = 0U;
    uint64_t hardware_max = 0U;
    uint64_t observed_min = 0U;
    uint64_t observed_max = 0U;

    if (!read_khz_value(policy_path + "/cpuinfo_min_freq", hardware_min) ||
        !read_khz_value(policy_path + "/cpuinfo_max_freq", hardware_max) ||
        !read_khz_value(policy_path + "/scaling_min_freq", observed_min) ||
        !read_khz_value(policy_path + "/scaling_max_freq", observed_max) ||
        hardware_min > hardware_max || observed_min > observed_max ||
        observed_min < hardware_min || observed_max > hardware_max) {
        return std::nullopt;
    }

    std::vector<uint64_t> frequencies;
    if (!read_frequency_table(policy_path, frequencies) ||
        frequencies.empty() || frequencies.size() > kMaxOperatingPoints) {
        return std::nullopt;
    }

    FrequencyCapabilitySnapshot capability{};
    capability.domain = FrequencyDomain::CpuPolicy;
    capability.hardware_min_hz = hardware_min;
    capability.hardware_max_hz = hardware_max;
    capability.observed_policy_min_hz = observed_min;
    capability.observed_policy_max_hz = observed_max;
    capability.hardware_limit_valid = true;
    capability.kernel_frequency_table_valid = true;

    capability.active_constraints_valid =
        request.active_constraints_valid &&
        request.active_constraint_min_hz >= hardware_min &&
        request.active_constraint_min_hz <= request.active_constraint_max_hz &&
        request.active_constraint_max_hz <= hardware_max;

    if (capability.active_constraints_valid) {
        capability.active_constraint_min_hz = request.active_constraint_min_hz;
        capability.active_constraint_max_hz = request.active_constraint_max_hz;
    }

    capability.thermal_admission_valid = request.thermal_admission_valid;
    capability.thermal_admission_granted = request.thermal_admission_granted;
    capability.capability_fingerprint_valid = request.capability_fingerprint_valid;

    capability.operating_points.reserve(frequencies.size());
    for (const uint64_t hz : frequencies) {
        const bool within_hardware =
            hz >= hardware_min && hz <= hardware_max;
        const bool permitted =
            capability.active_constraints_valid &&
            hz >= capability.active_constraint_min_hz &&
            hz <= capability.active_constraint_max_hz;

        capability.operating_points.push_back({
            hz,
            true,
            within_hardware,
            permitted
        });
    }

    return capability;
}

bool FrequencyTargetPlanner::validate(
    const FrequencyCapabilitySnapshot& capability) noexcept {
    if (capability.domain != FrequencyDomain::CpuPolicy ||
        !capability.hardware_limit_valid ||
        !capability.active_constraints_valid ||
        !capability.kernel_frequency_table_valid ||
        !capability.thermal_admission_valid ||
        !capability.thermal_admission_granted ||
        !capability.capability_fingerprint_valid ||
        capability.hardware_min_hz == 0U ||
        capability.hardware_min_hz > capability.hardware_max_hz ||
        capability.active_constraint_min_hz < capability.hardware_min_hz ||
        capability.active_constraint_min_hz > capability.active_constraint_max_hz ||
        capability.active_constraint_max_hz > capability.hardware_max_hz ||
        capability.observed_policy_min_hz < capability.hardware_min_hz ||
        capability.observed_policy_min_hz > capability.observed_policy_max_hz ||
        capability.observed_policy_max_hz > capability.hardware_max_hz ||
        capability.operating_points.empty() ||
        capability.operating_points.size() > kMaxOperatingPoints) {
        return false;
    }

    uint64_t previous_hz = 0U;
    bool has_eligible_opp = false;
    for (const auto& point : capability.operating_points) {
        if (point.hz == 0U || point.hz <= previous_hz || !point.exposed_by_kernel) {
            return false;
        }
        previous_hz = point.hz;

        const bool expected_hardware =
            point.hz >= capability.hardware_min_hz &&
            point.hz <= capability.hardware_max_hz;
        const bool expected_constraint =
            point.hz >= capability.active_constraint_min_hz &&
            point.hz <= capability.active_constraint_max_hz;

        if (point.within_hardware_limit != expected_hardware ||
            point.permitted_by_active_constraints != expected_constraint) {
            return false;
        }
        has_eligible_opp = has_eligible_opp ||
                           (expected_hardware && expected_constraint);
    }
    return has_eligible_opp;
}

std::optional<uint64_t> FrequencyTargetPlanner::select_target(
    const FrequencyCapabilitySnapshot& capability,
    uint64_t current_hz,
    double requested_capacity_scale) const noexcept {
    if (!validate(capability) || current_hz == 0U ||
        !std::isfinite(requested_capacity_scale) ||
        requested_capacity_scale < kMinimumCapacityScale ||
        requested_capacity_scale > kMaximumCapacityScale) {
        return std::nullopt;
    }

    const long double scaled =
        static_cast<long double>(current_hz) *
        static_cast<long double>(requested_capacity_scale);
    if (!std::isfinite(scaled) || scaled <= 0.0L) {
        return std::nullopt;
    }

    uint64_t target_hz = 0U;
    const long double bounded = std::clamp(
        scaled,
        static_cast<long double>(capability.active_constraint_min_hz),
        static_cast<long double>(capability.active_constraint_max_hz));
    if (bounded >= static_cast<long double>(capability.active_constraint_max_hz)) {
        target_hz = capability.active_constraint_max_hz;
    } else {
        target_hz = static_cast<uint64_t>(bounded);
    }

    std::optional<uint64_t> lower;
    std::optional<uint64_t> upper;
    for (const auto& point : capability.operating_points) {
        if (!point.within_hardware_limit || !point.permitted_by_active_constraints) {
            continue;
        }
        if (point.hz <= target_hz) {
            lower = point.hz;
        }
        if (point.hz >= target_hz && !upper.has_value()) {
            upper = point.hz;
        }
    }

    if (requested_capacity_scale >= 1.0) {
        return upper.has_value() ? upper : lower;
    }
    return lower.has_value() ? lower : upper;
}

} // namespace coreflow
