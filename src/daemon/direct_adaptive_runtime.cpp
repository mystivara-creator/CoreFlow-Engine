/**
 * @file direct_adaptive_runtime.cpp
 * @brief Runtime implementation for the five-module Direct Adaptive architecture.
 *
 * Discovery is semantic and capability based. The policy consumes only
 * discovered ABI nodes, advertised OPPs and normalized telemetry; it does not
 * synthesize a device profile or use a vendor-specific frequency table.
 */
#include "coreflow/direct_adaptive_runtime.hpp"

#include <algorithm>
#include <cctype>
#include <type_traits>
#include <array>
#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fcntl.h>
#include <fstream>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <string_view>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <utility>

namespace coreflow::runtime {
namespace fs = std::filesystem;

namespace {

constexpr std::size_t kReadLimit = 4096U;

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1U);
}

std::string read_text(const fs::path& path) {
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return {};
    struct stat st{};
    if (::fstat(fd, &st) != 0 || !S_ISREG(st.st_mode)) {
        (void)::close(fd);
        return {};
    }
    std::string value;
    value.reserve(std::min<std::size_t>(static_cast<std::size_t>(std::max<off_t>(0, st.st_size)), kReadLimit));
    char buffer[512];
    while (value.size() < kReadLimit) {
        const ssize_t n = ::read(fd, buffer, std::min(sizeof(buffer), kReadLimit - value.size()));
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
        value.append(buffer, static_cast<std::size_t>(n));
        if (value.find('\n') != std::string::npos) break;
    }
    (void)::close(fd);
    const auto newline = value.find('\n');
    if (newline != std::string::npos) value.resize(newline);
    return trim(std::move(value));
}

bool parse_i64(std::string_view text, std::int64_t& value) noexcept {
    if (text.empty() || text.size() > 32U) return false;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    return result.ec == std::errc{} && result.ptr == text.data() + text.size();
}

bool parse_u64(std::string_view text, std::uint64_t& value) noexcept {
    if (text.empty() || text.size() > 32U) return false;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    return result.ec == std::errc{} && result.ptr == text.data() + text.size();
}

bool parse_double(std::string_view text, double& value) noexcept {
    if (text.empty() || text.size() > 48U) return false;
    std::string copy(text);
    char* end = nullptr;
    errno = 0;
    value = std::strtod(copy.c_str(), &end);
    return errno == 0 && end == copy.c_str() + copy.size() && std::isfinite(value);
}

NodeValue parse_node_value(const std::string& text) {
    std::int64_t signed_value = 0;
    if (parse_i64(text, signed_value)) return signed_value;
    double floating = 0.0;
    if (parse_double(text, floating)) return floating;
    return text;
}

std::string node_value_text(const NodeValue& value) {
    return std::visit([](const auto& item) -> std::string {
        using T = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<T, std::monostate>) return {};
        else if constexpr (std::is_same_v<T, std::string>) return item;
        else if constexpr (std::is_same_v<T, bool>) return item ? "1" : "0";
        else if constexpr (std::is_floating_point_v<T>) {
            char buffer[64];
            const int n = std::snprintf(buffer, sizeof(buffer), "%.12g", static_cast<double>(item));
            return n > 0 ? std::string(buffer, static_cast<std::size_t>(n)) : std::string{};
        } else return std::to_string(item);
    }, value);
}

bool numeric_value(const NodeValue& value, long double& result) noexcept {
    return std::visit([&](const auto& item) noexcept -> bool {
        using T = std::decay_t<decltype(item)>;
        if constexpr (std::is_arithmetic_v<T>) {
            result = static_cast<long double>(item);
            return std::isfinite(static_cast<double>(result));
        }
        return false;
    }, value);
}

std::string json_escape(std::string_view value) {
    std::string out{"\""};
    for (const unsigned char ch : value) {
        switch (ch) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (ch >= 0x20U) out.push_back(static_cast<char>(ch));
                break;
        }
    }
    out.push_back('"');
    return out;
}

std::string mode_name(OperatingMode mode) {
    switch (mode) {
        case OperatingMode::Idle: return "SCREEN_OFF";
        case OperatingMode::Balanced: return "BALANCED_SWEET_SPOT";
        case OperatingMode::HighLoad: return "ACTIVE_LOAD";
        case OperatingMode::ThermalGuard: return "THERMAL_GUARD";
        default: return "DISCOVERY";
    }
}

std::string actuator_health_name(ActuatorHealth health) {
    switch (health) {
        case ActuatorHealth::Healthy: return "VERIFIED";
        case ActuatorHealth::SkippedUnavailable: return "SKIPPED_UNAVAILABLE";
        case ActuatorHealth::PermissionDenied: return "PERMISSION_DENIED";
        case ActuatorHealth::WriteFailed: return "WRITE_FAILED";
        case ActuatorHealth::ReadbackFailed: return "READBACK_FAILED";
        case ActuatorHealth::ValueMismatch: return "VALUE_MISMATCH";
        case ActuatorHealth::OemContention: return "OEM_CONTENTION";
        case ActuatorHealth::IdentityLost: return "IDENTITY_LOST";
        case ActuatorHealth::Quarantined: return "QUARANTINED";
    }
    return "UNKNOWN";
}

std::vector<std::uint64_t> parse_ordered_integer_list(std::string_view raw) {
    std::vector<std::uint64_t> values;
    std::istringstream stream{std::string(raw)};
    std::string token;
    while (stream >> token && values.size() < 512U) {
        std::uint64_t parsed = 0;
        if (!parse_u64(token, parsed) || parsed == 0U) return {};
        values.push_back(parsed);
    }
    return values;
}

std::vector<std::uint64_t> parse_integer_list(std::string_view raw) {
    std::vector<std::uint64_t> values;
    std::istringstream stream{std::string(raw)};
    std::string token;
    while (stream >> token && values.size() < 512U) {
        std::uint64_t parsed = 0;
        if (parse_u64(token, parsed) && parsed != 0U) values.push_back(parsed);
    }
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    return values;
}

std::vector<NodeValue> as_node_values(const std::vector<std::uint64_t>& values) {
    std::vector<NodeValue> output;
    output.reserve(values.size());
    for (const auto value : values) output.emplace_back(static_cast<std::int64_t>(value));
    return output;
}

fs::path canonical_under(const fs::path& input, const fs::path& root) {
    std::error_code ec;
    const auto canonical = fs::canonical(input, ec);
    if (ec) return {};
    const auto canonical_root = fs::canonical(root, ec);
    if (ec) return {};
    const auto relative = canonical.lexically_relative(canonical_root);
    if (relative.empty() || relative.native().rfind("..", 0) == 0) return {};
    return canonical;
}

std::vector<fs::path> child_directories(const fs::path& parent, std::size_t limit = 256U) {
    std::vector<fs::path> output;
    std::error_code ec;
    fs::directory_iterator it(parent, fs::directory_options::skip_permission_denied, ec);
    const fs::directory_iterator end;
    for (; !ec && it != end && output.size() < limit; it.increment(ec)) {
        std::error_code item_error;
        if (!it->is_directory(item_error) || item_error) continue;
        output.push_back(it->path());
    }
    std::sort(output.begin(), output.end());
    return output;
}

std::vector<fs::path> find_tunables(const fs::path& root, std::size_t max_depth = 3U) {
    std::vector<fs::path> output;
    std::error_code ec;
    fs::recursive_directory_iterator it(
        root, fs::directory_options::skip_permission_denied, ec), end;
    for (; !ec && it != end && output.size() < 256U; it.increment(ec)) {
        if (it.depth() > static_cast<int>(max_depth)) {
            it.disable_recursion_pending();
            continue;
        }
        std::error_code status_error;
        const auto status = it->symlink_status(status_error);
        if (status_error || fs::is_symlink(status)) continue;
        if (fs::is_regular_file(status)) output.push_back(it->path());
    }
    return output;
}

std::optional<std::uint64_t> parse_refresh_from_text(std::string_view text) noexcept {
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] != '@') continue;
        std::size_t end = i + 1U;
        while (end < text.size() && text[end] >= '0' && text[end] <= '9') ++end;
        std::uint64_t hz = 0U;
        if (parse_u64(text.substr(i + 1U, end - i - 1U), hz) && hz > 0U && hz <= 1000U) return hz;
    }
    return std::nullopt;
}

std::optional<double> read_psi_avg10(const fs::path& path) {
    const auto text = read_text(path);
    const auto position = text.find("avg10=");
    if (position == std::string::npos) return std::nullopt;
    const auto start = position + 6U;
    const auto end = text.find_first_of(" \t", start);
    double value = 0.0;
    const auto token = std::string_view(text).substr(start,
        end == std::string::npos ? text.size() - start : end - start);
    if (!parse_double(token, value) || value < 0.0 || value > 100.0) return std::nullopt;
    return value;
}

FileIdentity identity_for(const fs::path& path) noexcept {
    struct stat st{};
    if (::stat(path.c_str(), &st) != 0) return {};
    return {static_cast<std::uint64_t>(st.st_dev), static_cast<std::uint64_t>(st.st_ino)};
}

std::optional<NumericRange> range_from_opp(const std::vector<std::uint64_t>& opps) {
    if (opps.size() < 2U) return std::nullopt;
    return NumericRange{static_cast<long double>(opps.front()),
                        static_cast<long double>(opps.back()), std::nullopt};
}

const NormalizedMetric* metric(const NormalizedState& state, std::string_view id) noexcept {
    return state.find(id);
}

double metric_value(const NormalizedState& state, std::string_view id, double fallback = 0.0) noexcept {
    const auto* found = metric(state, id);
    return found != nullptr && found->well_formed() ? found->value : fallback;
}

} // namespace

PolicyIntent calculate_policy_intent(const PolicyInput& input, Timestamp evaluation_time) noexcept {
    PolicyIntent intent;
    intent.evaluated_at = evaluation_time;
    const auto& state = input.state;

    const auto* screen_metric = metric(state, "screen.off");
    const bool screen_off = screen_metric != nullptr && screen_metric->well_formed() &&
                            screen_metric->value >= 0.5;
    const double cpu_load = metric_value(state, "cpu.util");
    const double thermal_pressure = metric_value(state, "thermal.pressure");
    const bool thermal_valid = metric(state, "thermal.pressure") != nullptr &&
                               metric(state, "thermal.pressure")->well_formed();
    const bool thermal_emergency = metric_value(state, "thermal.emergency") >= 0.5;
    const bool high_load = cpu_load >= 0.70 ||
        metric_value(state, "cpu.pressure") >= 0.20 ||
        metric_value(state, "gpu.busy") >= 0.70;

    if (thermal_emergency || (thermal_valid && thermal_pressure >= 0.85)) {
        intent.mode = OperatingMode::ThermalGuard;
    } else if (screen_off) {
        intent.mode = OperatingMode::Idle;
    } else if (high_load) {
        intent.mode = OperatingMode::HighLoad;
    } else {
        intent.mode = OperatingMode::Balanced;
    }

    const auto* refresh_metric = metric(state, "display.refresh_hz");
    const bool refresh_valid = refresh_metric != nullptr && refresh_metric->well_formed();
    const double refresh_hz = refresh_valid ? refresh_metric->value * 240.0 : 0.0;
    const double frame_us = refresh_hz > 0.0 ? 1000000.0 / refresh_hz : 0.0;
    const bool ram_ge_6gb = metric_value(state, "device.ram_ge_6gb") >= 0.5;
    const bool zram_active = metric_value(state, "device.zram_active") >= 0.5;

    for (const auto& control : input.controls) {
        if (!control.writable || !control.supported_range.valid() ||
            std::holds_alternative<std::monostate>(control.current_value)) continue;

        const NodeDescriptor* node = input.manifest.find(control.node_id);
        if (node == nullptr || node->access != AccessState::Writable ||
            node->write_probe != ProbeState::Available) continue;

        NodeValue target;
        bool has_target = false;
        long double numeric_target = 0.0L;
        const std::string_view semantic(node->semantic);

        const auto choose_opp = [&](double fraction, bool kgsl_reverse = false) -> std::optional<std::uint64_t> {
            std::vector<std::uint64_t> values;
            values.reserve(node->advertised_values.size());
            for (const auto& advertised : node->advertised_values) {
                long double value = 0.0L;
                if (numeric_value(advertised, value) &&
                    (kgsl_reverse ? value >= 0.0L : value > 0.0L)) {
                    values.push_back(static_cast<std::uint64_t>(value));
                }
            }
            if (values.empty()) return std::nullopt;
            if (kgsl_reverse) {
                const auto max_index = values.size() - 1U;
                const auto index = static_cast<std::size_t>(std::llround(
                    static_cast<double>(max_index) * std::clamp(fraction, 0.0, 1.0)));
                return values[std::min(index, max_index)];
            }
            std::sort(values.begin(), values.end());
            const auto index = static_cast<std::size_t>(std::llround(
                static_cast<double>(values.size() - 1U) * std::clamp(fraction, 0.0, 1.0)));
            return values[std::min(index, values.size() - 1U)];
        };

        if (semantic == "cpu.hispeed_freq") {
            const double tier = intent.mode == OperatingMode::Idle ? 0.35 :
                intent.mode == OperatingMode::HighLoad ? 0.78 :
                intent.mode == OperatingMode::ThermalGuard ? 0.58 : 0.70;
            if (const auto selected = choose_opp(tier)) {
                numeric_target = static_cast<long double>(*selected);
                has_target = true;
            }
        } else if (semantic == "cpu.hispeed_load") {
            const bool little = node->domain_id.find("little") != std::string::npos;
            const bool big = node->domain_id.find("big") != std::string::npos;
            if (intent.mode == OperatingMode::Idle) numeric_target = 95.0L;
            else if (intent.mode == OperatingMode::HighLoad) numeric_target = 75.0L;
            else if (intent.mode == OperatingMode::ThermalGuard) numeric_target = 90.0L;
            else if (little) numeric_target = 90.0L;
            else if (big) numeric_target = 82.0L;
            else continue;
            has_target = true;
        } else if (semantic == "cpu.up_rate_limit_us" && frame_us > 0.0) {
            numeric_target = std::clamp(frame_us * 0.06, 100.0, 5000.0);
            has_target = true;
        } else if (semantic == "cpu.down_rate_limit_us" && frame_us > 0.0) {
            numeric_target = std::clamp(frame_us * 1.2, 10000.0, 20000.0);
            has_target = true;
        } else if (semantic == "cpu.boost") {
            long double current = 0.0L;
            if (numeric_value(control.current_value, current) && current < 0.0L) {
                numeric_target = 0.0L;
                has_target = true;
            }
        } else if (semantic == "gpu.idle_timer" && frame_us > 0.0) {
            const double window = intent.mode == OperatingMode::Idle ? 8.0 :
                intent.mode == OperatingMode::HighLoad ? 20.0 :
                intent.mode == OperatingMode::ThermalGuard ? 8.0 : 14.0;
            numeric_target = (frame_us / 1000.0) * window;
            has_target = true;
        } else if (semantic == "gpu.kgsl.default_pwrlevel") {
            const double tier = intent.mode == OperatingMode::Idle ? 0.72 :
                intent.mode == OperatingMode::HighLoad ? 0.20 :
                intent.mode == OperatingMode::ThermalGuard ? 0.80 : 0.35;
            if (const auto selected = choose_opp(tier, true)) {
                numeric_target = static_cast<long double>(*selected);
                has_target = true;
            }
        } else if (semantic == "gpu.kgsl.min_pwrlevel") {
            const double tier = intent.mode == OperatingMode::Idle ? 0.72 :
                intent.mode == OperatingMode::HighLoad ? 0.20 :
                intent.mode == OperatingMode::ThermalGuard ? 0.80 : 0.35;
            if (const auto selected = choose_opp(tier, true)) {
                numeric_target = static_cast<long double>(*selected);
                has_target = true;
            }
        } else if (semantic == "gpu.devfreq.max_freq") {
            const double tier = intent.mode == OperatingMode::Idle ? 0.40 :
                intent.mode == OperatingMode::HighLoad ? 0.95 :
                intent.mode == OperatingMode::ThermalGuard ? 0.62 : 0.80;
            if (const auto selected = choose_opp(tier)) {
                numeric_target = static_cast<long double>(*selected);
                has_target = true;
            }
        } else if (semantic == "vm.swappiness") {
            // ZRAM-backed systems with larger RAM retain a moderate reclaim
            // bias; smaller memory configurations derive a stronger reclaim bias.
            const long double value = ram_ge_6gb && zram_active ? 65.0L :
                ram_ge_6gb ? 70.0L : 82.0L;
            numeric_target = value;
            has_target = true;
        } else if (semantic == "vm.page_cluster") {
            numeric_target = 0.0L;
            has_target = true;
        } else if (semantic == "storage.read_ahead_kb") {
            if (node->domain_id.find("ufs") != std::string::npos ||
                node->domain_id.find("nvme") != std::string::npos) {
                numeric_target = 128.0L;
                has_target = true;
            } else if (node->domain_id.find("emmc") != std::string::npos) {
                numeric_target = 256.0L;
                has_target = true;
            }
        } else if (semantic == "storage.iostats") {
            numeric_target = 0.0L;
            has_target = true;
        }

        if (!has_target || !std::isfinite(static_cast<double>(numeric_target))) continue;
        numeric_target = std::clamp(numeric_target,
            control.supported_range.minimum, control.supported_range.maximum);
        if (control.value_kind == ValueKind::FloatingPoint) {
            target = static_cast<double>(numeric_target);
        } else {
            target = static_cast<std::int64_t>(std::llround(static_cast<double>(numeric_target)));
        }
        if (target == control.current_value) continue;

        intent.actions.push_back({
            control.node_id,
            control.current_value,
            std::move(target),
            "device-capability-derived Direct Sweet-Spot policy"
        });
    }

    if (!refresh_valid) intent.unavailable_evidence.emplace_back("display.refresh_rate");
    if (!thermal_valid) intent.unavailable_evidence.emplace_back("thermal_pressure");
    if (state.find("cpu.util") == nullptr) intent.unavailable_evidence.emplace_back("cpu.utilization");
    return intent;
}

DiscoveryResult TopologyScanner::discover(const DiscoveryRoots& roots) noexcept {
    DiscoveryResult result;
    auto& manifest = result.manifest;
    manifest.generated_at = Clock::now();

    const auto add_node = [&](const fs::path& path, NodeDomain domain,
                              std::string domain_id, std::string semantic,
                              std::optional<NumericRange> range = std::nullopt,
                              std::vector<NodeValue> advertised = {}) {
        const std::string raw = read_text(path);
        if (raw.empty()) return;
        struct stat st{};
        if (::stat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) return;

        NodeDescriptor node;
        node.id = semantic + "@" + std::to_string(manifest.nodes.size());
        node.discovered_path = path;
        node.domain = domain;
        node.domain_id = std::move(domain_id);
        node.semantic = std::move(semantic);
        node.observed_value = parse_node_value(raw);
        node.value_kind = std::holds_alternative<std::int64_t>(node.observed_value)
            ? ValueKind::SignedInteger :
            std::holds_alternative<double>(node.observed_value)
                ? ValueKind::FloatingPoint : ValueKind::Text;
        node.access = ::access(path.c_str(), W_OK) == 0 ? AccessState::Writable : AccessState::ReadOnly;
        node.read_probe = ProbeState::Available;
        node.write_probe = node.access == AccessState::Writable
            ? ProbeState::Available : ProbeState::PermissionDenied;
        node.identity = identity_for(path);
        node.numeric_range = range;
        node.advertised_values = std::move(advertised);
        node.discovered_at = Clock::now();
        manifest.nodes.push_back(std::move(node));
    };

    const fs::path cpufreq_root = roots.sysfs_root / "devices/system/cpu/cpufreq";
    const auto policies = child_directories(cpufreq_root);
    std::vector<std::uint64_t> policy_maxima;
    struct CpuPolicyInfo final { fs::path path; std::string domain; std::vector<std::uint64_t> opps; };
    std::vector<CpuPolicyInfo> cpu_infos;
    for (const auto& policy : policies) {
        const auto name = policy.filename().string();
        if (name.rfind("policy", 0) != 0) continue;
        auto opps = parse_integer_list(read_text(policy / "scaling_available_frequencies"));
        if (opps.size() < 2U) continue;
        policy_maxima.push_back(opps.back());
        cpu_infos.push_back({policy, {}, std::move(opps)});
    }
    const auto max_policy = policy_maxima.empty() ? 0U :
        *std::max_element(policy_maxima.begin(), policy_maxima.end());

    constexpr std::array<std::string_view, 5> cpu_tunables{
        "hispeed_freq", "hispeed_load", "up_rate_limit_us",
        "down_rate_limit_us", "boost"};
    for (auto& info : cpu_infos) {
        const bool little = max_policy != 0U && info.opps.back() < max_policy;
        info.domain = little ? "cpu.little" : "cpu.big";
        const auto governor = read_text(info.path / "scaling_governor");
        if (governor != "walt" && governor != "schedutil") continue;
        const auto files = find_tunables(info.path);
        for (const auto& candidate : files) {
            const auto leaf = candidate.filename().string();
            if (std::find(cpu_tunables.begin(), cpu_tunables.end(), leaf) == cpu_tunables.end()) continue;
            std::string semantic;
            if (leaf == "hispeed_freq") semantic = "cpu.hispeed_freq";
            else if (leaf == "hispeed_load") semantic = "cpu.hispeed_load";
            else if (leaf == "up_rate_limit_us") semantic = "cpu.up_rate_limit_us";
            else if (leaf == "down_rate_limit_us") semantic = "cpu.down_rate_limit_us";
            else semantic = "cpu.boost";
            std::optional<NumericRange> range;
            if (leaf == "hispeed_freq") range = range_from_opp(info.opps);
            else if (leaf == "hispeed_load") range = NumericRange{0.0L, 100.0L, 1.0L};
            else if (leaf == "boost") range = NumericRange{-128.0L, 128.0L, 1.0L};
            else range = NumericRange{0.0L, 1000000.0L, 1.0L};
            add_node(candidate, NodeDomain::Cpu, info.domain, semantic, range,
                     leaf == "hispeed_freq" ? as_node_values(info.opps) : std::vector<NodeValue>{});
        }
    }

    // KGSL semantic discovery: the class entry is resolved to its actual sysfs
    // object before any path is admitted to the manifest.
    const fs::path kgsl_root = roots.sysfs_root / "class/kgsl";
    for (const auto& entry : child_directories(kgsl_root)) {
        const auto resolved = canonical_under(entry, roots.sysfs_root);
        if (resolved.empty()) continue;
        const auto freqs = parse_ordered_integer_list(read_text(resolved / "gpu_available_frequencies"));
        const auto levels_raw = read_text(resolved / "num_pwrlevels");
        std::uint64_t levels = 0U;
        if (freqs.size() < 2U || !parse_u64(levels_raw, levels) || levels != freqs.size()) continue;
        const bool increasing = std::is_sorted(freqs.begin(), freqs.end());
        const bool decreasing = std::is_sorted(freqs.rbegin(), freqs.rend());
        if (!increasing && !decreasing) continue;
        std::vector<NodeValue> level_values;
        level_values.reserve(static_cast<std::size_t>(levels));
        for (std::uint64_t i = 0; i < levels; ++i) level_values.emplace_back(static_cast<std::int64_t>(i));
        const auto model = read_text(resolved / "gpu_model");
        const std::string gpu_id = "gpu.kgsl:" + (model.empty() ? entry.filename().string() : model);
        const auto level_range = NumericRange{0.0L, static_cast<long double>(levels - 1U), 1.0L};
        const auto default_path = resolved / "default_pwrlevel";
        if (!read_text(default_path).empty()) {
            add_node(default_path, NodeDomain::Gpu, gpu_id,
                     "gpu.kgsl.default_pwrlevel", level_range, level_values);
        } else {
            add_node(resolved / "min_pwrlevel", NodeDomain::Gpu, gpu_id,
                     "gpu.kgsl.min_pwrlevel", level_range, level_values);
        }
        add_node(resolved / "idle_timer", NodeDomain::Gpu, gpu_id,
                 "gpu.idle_timer", NumericRange{0.0L, 1000000.0L, 1.0L});
    }

    // Generic devfreq GPU providers (Mali and other implementations) are
    // selected from the driver's reported name/compatible string, not its path.
    const fs::path devfreq_root = roots.sysfs_root / "class/devfreq";
    for (const auto& entry : child_directories(devfreq_root)) {
        const auto resolved = canonical_under(entry, roots.sysfs_root);
        if (resolved.empty()) continue;
        std::string reported = read_text(resolved / "name");
        if (reported.empty()) reported = read_text(resolved / "device/of_node/compatible");
        if (reported.empty()) reported = read_text(resolved / "device/uevent");
        std::string folded = reported;
        std::transform(folded.begin(), folded.end(), folded.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (folded.find("gpu") == std::string::npos &&
            folded.find("mali") == std::string::npos &&
            folded.find("adreno") == std::string::npos &&
            folded.find("kgsl") == std::string::npos) continue;
        const std::string gpu_id = "gpu.devfreq:" + (reported.empty() ? entry.filename().string() : reported);
        const auto opps = parse_integer_list(read_text(resolved / "available_frequencies"));
        const auto range = range_from_opp(opps);
        if (range) {
            add_node(resolved / "max_freq", NodeDomain::Gpu, gpu_id,
                     "gpu.devfreq.max_freq", range, as_node_values(opps));
        }
        add_node(resolved / "idle_timer", NodeDomain::Gpu, gpu_id,
                 "gpu.idle_timer", NumericRange{0.0L, 1000000.0L, 1.0L});
    }

    const fs::path vm_root = roots.procfs_root / "sys/vm";
    add_node(vm_root / "swappiness", NodeDomain::Memory, "vm",
             "vm.swappiness", NumericRange{0.0L, 200.0L, 1.0L});
    add_node(vm_root / "page-cluster", NodeDomain::Memory, "vm",
             "vm.page_cluster", NumericRange{0.0L, 8.0L, 1.0L});

    // Physical storage is identified through resolved device topology. Virtual
    // disks (dm, loop, ram and zram) are deliberately excluded.
    const fs::path block_root = roots.sysfs_root / "class/block";
    for (const auto& entry : child_directories(block_root)) {
        const auto resolved = canonical_under(entry, roots.sysfs_root);
        if (resolved.empty() || resolved.string().find("/devices/virtual/") != std::string::npos) continue;
        const auto queue = resolved / "queue";
        if (read_text(queue / "rotational") != "0") continue;
        std::string topology = resolved.string();
        std::transform(topology.begin(), topology.end(), topology.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        std::string storage = "solid_state";
        if (topology.find("ufshc") != std::string::npos || topology.find("ufs") != std::string::npos)
            storage = "ufs";
        else if (topology.find("mmc") != std::string::npos)
            storage = "emmc";
        else if (topology.find("nvme") != std::string::npos)
            storage = "nvme";
        const std::string storage_id = "storage." + storage + ":" + entry.filename().string();
        add_node(queue / "read_ahead_kb", NodeDomain::Storage, storage_id,
                 "storage.read_ahead_kb", NumericRange{0.0L, 65536.0L, 1.0L});
        add_node(queue / "iostats", NodeDomain::Storage, storage_id,
                 "storage.iostats", NumericRange{0.0L, 1.0L, 1.0L});
    }

    result.enumeration_completed = !manifest.nodes.empty();
    if (manifest.nodes.empty()) {
        manifest.issues.push_back({"sysfs", ProbeState::Missing,
            "No supported adaptive control nodes were discovered; daemon lifecycle remains available."});
    }
    return result;
}

OemBaseline Telemetry::capture_oem_baseline(const DeviceManifest& manifest) noexcept {
    OemBaseline baseline;
    baseline.captured_at = Clock::now();
    baseline.boot_identity = baseline_.boot_id();
    for (const auto& node : manifest.nodes) {
        if (node.access != AccessState::Writable || node.read_probe != ProbeState::Available) continue;
        if (!baseline_.capture_durable(node.discovered_path.string())) continue;
        const auto audit = baseline_.get_audit_trail();
        const auto found = std::find_if(audit.begin(), audit.end(), [&](const NodeSnapshot& item) { return item.path == node.discovered_path.string(); });
        if (found != audit.end()) baseline.entries.push_back({node.id, parse_node_value(found->factory_value), {found->dev, found->ino}, baseline.captured_at});
    }
    return baseline;
}

NormalizedState Telemetry::normalize(
    const DeviceManifest&, const TelemetrySnapshot& telemetry) noexcept {
    NormalizedState state;
    state.sampled_at = telemetry.sampled_at;
    for (const auto& sample : telemetry.samples) {
        if (!sample.valid || !sample.discovered_range || !sample.discovered_range->valid()) continue;
        long double raw = 0.0L;
        if (!numeric_value(sample.value, raw)) continue;
        const auto& range = *sample.discovered_range;
        const long double normalized = std::clamp(
            (raw - range.minimum) / (range.maximum - range.minimum), 0.0L, 1.0L);
        state.metrics.push_back({
            sample.metric_id, static_cast<double>(normalized), 1.0,
            sample.sampled_at, true
        });
    }
    return state;
}

TelemetrySnapshot Telemetry::sample(
    const DeviceManifest&, bool& screen_off, std::uint64_t& ram_bytes,
    std::uint64_t& zram_bytes, std::uint32_t& refresh_hz,
    double& cpu_utilization) noexcept {
    TelemetrySnapshot snapshot;
    snapshot.sampled_at = Clock::now();
    const auto add = [&](std::string id, double value, double minimum, double maximum, bool valid = true) {
        snapshot.samples.push_back({
            std::move(id), value, snapshot.sampled_at,
            NumericRange{minimum, maximum, std::nullopt}, valid
        });
    };

    const std::string stat = read_text("/proc/stat");
    std::istringstream cpu_stream(stat);
    std::string label;
    std::uint64_t user = 0, nice = 0, system = 0, idle = 0, iowait = 0;
    std::uint64_t irq = 0, softirq = 0, steal = 0;
    bool cpu_valid = false;
    if (cpu_stream >> label >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal && label == "cpu") {
        const std::uint64_t total = user + nice + system + idle + iowait + irq + softirq + steal;
        const std::uint64_t idle_total = idle + iowait;
        if (previous_cpu_total_ != 0U && total > previous_cpu_total_ && idle_total >= previous_cpu_idle_) {
            const auto total_delta = total - previous_cpu_total_;
            const auto idle_delta = idle_total - previous_cpu_idle_;
            cpu_utilization = std::clamp(
                1.0 - static_cast<double>(idle_delta) / static_cast<double>(total_delta), 0.0, 1.0);
            cpu_valid = true;
        }
        previous_cpu_total_ = total;
        previous_cpu_idle_ = idle_total;
    }
    add("cpu.util", cpu_utilization, 0.0, 1.0, cpu_valid);

    const std::string meminfo = read_text("/proc/meminfo");
    std::istringstream mem_stream(meminfo);
    std::string key;
    std::uint64_t amount = 0;
    std::string unit;
    ram_bytes = 0U;
    while (mem_stream >> key >> amount >> unit) {
        if (key == "MemTotal:") { ram_bytes = amount * 1024U; break; }
    }
    add("device.ram_ge_6gb", ram_bytes >= 6ULL * 1024ULL * 1024ULL * 1024ULL ? 1.0 : 0.0, 0.0, 1.0, ram_bytes > 0U);

    zram_bytes = 0U;
    bool zram_inventory_valid = false;
    for (const auto& block : child_directories("/sys/block")) {
        if (block.filename().string().rfind("zram", 0) != 0) continue;
        zram_inventory_valid = true;
        std::uint64_t bytes = 0U;
        if (parse_u64(read_text(block / "disksize"), bytes)) zram_bytes += bytes;
    }
    add("device.zram_active", zram_bytes > 0U ? 1.0 : 0.0,
        0.0, 1.0, zram_inventory_valid);

    screen_off = false;
    const auto backlights = child_directories("/sys/class/backlight");
    if (!backlights.empty()) {
        bool all_dark = true;
        bool have_brightness = false;
        for (const auto& backlight : backlights) {
            const auto resolved = canonical_under(backlight, "/sys");
            if (resolved.empty()) continue;
            const auto current = read_text(resolved / "brightness");
            const auto maximum = read_text(resolved / "max_brightness");
            std::uint64_t brightness = 0U, max_brightness = 0U;
            if (!parse_u64(current, brightness) || !parse_u64(maximum, max_brightness) || max_brightness == 0U) continue;
            have_brightness = true;
            if (brightness != 0U) all_dark = false;
        }
        screen_off = have_brightness && all_dark;
    }
    add("screen.off", screen_off ? 1.0 : 0.0, 0.0, 1.0);

    refresh_hz = 0U;
    const std::array<fs::path, 2> display_roots{
        fs::path("/sys/class/drm"), fs::path("/sys/class/graphics")
    };
    for (const auto& root : display_roots) {
        for (const auto& device : child_directories(root)) {
            const auto resolved = canonical_under(device, "/sys");
            if (resolved.empty()) continue;
            for (const auto& candidate : find_tunables(resolved, 2U)) {
                const auto leaf = candidate.filename().string();
                if (leaf != "available_modes" && leaf != "modes" &&
                    leaf != "current_mode" && leaf != "refresh_rate") continue;
                const auto text = read_text(candidate);
                std::uint64_t hz = 0U;
                if (leaf == "refresh_rate" && parse_u64(text, hz) && hz > 0U && hz <= 1000U) {
                    refresh_hz = static_cast<std::uint32_t>(hz);
                    break;
                }
                if (const auto parsed = parse_refresh_from_text(text)) {
                    refresh_hz = static_cast<std::uint32_t>(*parsed);
                    break;
                }
            }
            if (refresh_hz != 0U) break;
        }
        if (refresh_hz != 0U) break;
    }
    add("display.refresh_hz", static_cast<double>(refresh_hz), 0.0, 240.0, refresh_hz > 0U);

    for (const auto& [id, path] : std::array<std::pair<std::string_view, fs::path>, 3>{
            std::pair{"cpu.pressure", fs::path("/proc/pressure/cpu")},
            std::pair{"memory.pressure", fs::path("/proc/pressure/memory")},
            std::pair{"io.pressure", fs::path("/proc/pressure/io")}}) {
        const auto pressure = read_psi_avg10(path);
        if (pressure) add(std::string(id), *pressure, 0.0, 100.0);
    }
    return snapshot;
}

PolicyIntent SweetSpotEngine::evaluate(
    const DeviceManifest& manifest, const OemBaseline& baseline,
    const NormalizedState& state, std::uint64_t, std::uint64_t,
    std::uint32_t, std::string_view, OperatingMode,
    Timestamp evaluation_time) const noexcept {
    AlgorithmConstants algorithm{0.08, 0.20, 32U, kReadLimit};
    AdaptiveReference reference;
    reference.generated_at = evaluation_time;
    std::vector<ControlCapability> controls;
    controls.reserve(manifest.nodes.size());
    for (const auto& node : manifest.nodes) {
        if (!node.numeric_range) continue;
        const auto* original = baseline.find(node.id);
        controls.push_back({
            node.id,
            node.value_kind,
            *node.numeric_range,
            node.observed_value,
            original != nullptr ? original->value : node.observed_value,
            node.advertised_values,
            node.access == AccessState::Writable && node.write_probe == ProbeState::Available &&
                original != nullptr && original->identity.valid() && node.identity.valid()
        });
    }
    return calculate_policy_intent(
        PolicyInput{manifest, baseline, state, reference, controls, algorithm},
        evaluation_time);
}

JournalStatus KernelActuator::append_and_sync(std::span<const MutationRequest> requests) noexcept {
    if (journal_fault_latched_) return JournalStatus::JournalFault;

    const int dir_fd = ::open("/data/adb/coreflow", O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (dir_fd < 0) {
        journal_fault_latched_ = true;
        return JournalStatus::JournalFault;
    }
    const std::string temp_name = "direct-adaptive.wal.tmp." + std::to_string(::getpid());
    const int fd = ::openat(dir_fd, temp_name.c_str(),
        O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (fd < 0) {
        (void)::close(dir_fd);
        journal_fault_latched_ = true;
        return JournalStatus::JournalFault;
    }
    const auto hex = [](std::string_view value) {
        constexpr char digits[] = "0123456789abcdef";
        std::string output;
        output.reserve(value.size() * 2U);
        for (const unsigned char ch : value) {
            output.push_back(digits[ch >> 4U]);
            output.push_back(digits[ch & 0x0fU]);
        }
        return output;
    };
    bool ok = true;
    const std::string header = "CFAE-DA-WAL1\n";
    ok = ::write(fd, header.data(), header.size()) == static_cast<ssize_t>(header.size());
    for (const auto& request : requests) {
        const auto path = paths_.find(request.node_id);
        if (path == paths_.end()) { ok = false; break; }
        const std::string line = hex(request.node_id) + "\t" +
            hex(path->second) + "\t" +
            std::to_string(request.expected_identity.device) + "\t" +
            std::to_string(request.expected_identity.inode) + "\t" +
            hex(node_value_text(request.expected_current_value)) + "\t" +
            hex(node_value_text(request.target_value)) + "\n";
        std::size_t offset = 0U;
        while (ok && offset < line.size()) {
            const ssize_t written = ::write(fd, line.data() + offset, line.size() - offset);
            if (written < 0 && errno == EINTR) continue;
            if (written <= 0) { ok = false; break; }
            offset += static_cast<std::size_t>(written);
        }
        if (!ok) break;
    }
    if (ok) ok = ::fsync(fd) == 0;
    (void)::close(fd);
    if (ok) ok = ::renameat(dir_fd, temp_name.c_str(), dir_fd, "direct-adaptive.wal") == 0;
    if (ok) ok = ::fsync(dir_fd) == 0;
    if (!ok) (void)::unlinkat(dir_fd, temp_name.c_str(), 0);
    (void)::close(dir_fd);
    if (!ok) {
        journal_fault_latched_ = true;
        return JournalStatus::JournalFault;
    }
    return JournalStatus::Committed;
}

namespace {
bool clear_direct_adaptive_wal() noexcept {
    const int dir_fd = ::open("/data/adb/coreflow", O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (dir_fd < 0) return errno == ENOENT;
    if (::unlinkat(dir_fd, "direct-adaptive.wal", 0) != 0 && errno != ENOENT) {
        (void)::close(dir_fd);
        return false;
    }
    const bool synced = ::fsync(dir_fd) == 0;
    (void)::close(dir_fd);
    return synced;
}
}

NodeIoResult KernelActuator::read(const NodeDescriptor& node) noexcept {
    const auto raw = read_text(node.discovered_path);
    if (raw.empty()) return {NodeIoStatus::Missing, {}, {}};
    const auto identity = identity_for(node.discovered_path);
    if (!identity.valid()) return {NodeIoStatus::IdentityChanged, {}, identity};
    return {NodeIoStatus::Ok, parse_node_value(raw), identity};
}

NodeIoResult KernelActuator::write(
    const NodeDescriptor& node, const NodeValue& expected_current_value,
    const NodeValue& target_value, const FileIdentity& expected_identity) noexcept {
    const auto current = read(node);
    if (current.status != NodeIoStatus::Ok) return current;
    if (current.identity.device != expected_identity.device ||
        current.identity.inode != expected_identity.inode) {
        return {NodeIoStatus::IdentityChanged, current.value, current.identity};
    }
    if (current.value != expected_current_value ||
        !baseline_.owns_expected_state(node.discovered_path.string(), node_value_text(expected_current_value))) {
        return {NodeIoStatus::Contended, current.value, current.identity};
    }
    const auto target = node_value_text(target_value);
    if (target.empty() || !baseline_.apply_mutation(node.discovered_path.string(), target)) {
        const auto after = read(node);
        if (after.status == NodeIoStatus::Ok && after.identity.inode != expected_identity.inode)
            return {NodeIoStatus::IdentityChanged, after.value, after.identity};
        return {NodeIoStatus::IoError, after.value, after.identity};
    }
    return read(node);
}

VerificationResult KernelActuator::verify(
    const NodeDescriptor& node, const MutationRequest& request,
    const NodeIoResult& readback, const OemBaselineEntry* oem_baseline) noexcept {
    if (readback.status != NodeIoStatus::Ok) return {ActuatorHealth::ReadbackFailed, false};
    if (oem_baseline == nullptr || !oem_baseline->identity.valid() ||
        readback.identity.device != request.expected_identity.device ||
        readback.identity.inode != request.expected_identity.inode) {
        return {ActuatorHealth::IdentityLost, false};
    }
    if (readback.value != request.target_value ||
        !baseline_.owns_expected_state(node.discovered_path.string(), node_value_text(request.target_value))) {
        return {ActuatorHealth::ValueMismatch, false};
    }
    return {ActuatorHealth::Healthy, true};
}

void KernelActuator::record(const NodeId& node_id, ActuatorHealth health) noexcept {
    health_[node_id] = health;
}

MutationBatchResult KernelActuator::execute(
    const DeviceManifest& manifest, const OemBaseline& baseline,
    const PolicyIntent& intent, const AlgorithmConstants& algorithm) noexcept {
    if (journal_fault_latched_) {
        MutationBatchResult blocked;
        blocked.journal_status = JournalStatus::JournalFault;
        blocked.global_mutation_stop = true;
        return blocked;
    }
    std::vector<MutationRequest> requests;
    requests.reserve(intent.actions.size());
    paths_.clear();
    const auto stamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        Clock::now().time_since_epoch()).count();
    for (const auto& action : intent.actions) {
        const auto* node = manifest.find(action.node_id);
        if (node == nullptr) continue;
        const auto previous_health = health_.find(action.node_id);
        if (previous_health != health_.end() &&
            (previous_health->second == ActuatorHealth::OemContention ||
             previous_health->second == ActuatorHealth::IdentityLost ||
             previous_health->second == ActuatorHealth::PermissionDenied ||
             previous_health->second == ActuatorHealth::SkippedUnavailable)) {
            continue;
        }
        paths_[node->id] = node->discovered_path.string();
        requests.push_back({
            "sweetspot-" + std::to_string(stamp),
            action.node_id,
            node->identity,
            action.expected_current_value,
            action.target_value,
            action.rationale
        });
    }
    MutationBatchResult result =
        engine_.execute(manifest, baseline, requests, *this, *this, *this, *this, algorithm);
    if (result.global_mutation_stop || result.journal_status == JournalStatus::JournalFault) {
        journal_fault_latched_ = true;
        return result;
    }
    if (!clear_direct_adaptive_wal()) {
        journal_fault_latched_ = true;
        result.journal_status = JournalStatus::JournalFault;
        result.global_mutation_stop = true;
    }
    return result;
}

std::string KernelActuator::health_name(const NodeId& id) const {
    const auto found = health_.find(id);
    return found == health_.end() ? "NOT_APPLIED" : actuator_health_name(found->second);
}

struct DirectAdaptiveRuntime::Impl final {
    explicit Impl(BaselineStore& store)
        : baseline(store), telemetry(store), actuator(store) {}

    BaselineStore& baseline;
    TopologyScanner scanner;
    Telemetry telemetry;
    SweetSpotEngine controller;
    KernelActuator actuator;
    DeviceManifest manifest;
    OemBaseline oem;
    NormalizedState state;
    DirectAdaptiveSnapshot snapshot;
    AlgorithmConstants algorithm{0.08, 0.20, 32U, kReadLimit};
    std::unordered_map<NodeId, NodeValue> last_targets;
    std::unordered_map<NodeId, NodeValue> last_observed;
    Timestamp last_evaluation{};
    bool mutation_enabled{true};

    void build_json() {
        std::ostringstream out;
        out << "{\"active\":" << (snapshot.started ? "true" : "false")
            << ",\"discoveryComplete\":" << (snapshot.discovery_complete ? "true" : "false")
            << ",\"mode\":" << json_escape(mode_name(snapshot.mode))
            << ",\"screenState\":" << json_escape(snapshot.screen_off ? "OFF" : "ON_OR_UNKNOWN")
            << ",\"thermalGuard\":" << (snapshot.thermal_guard ? "true" : "false")
            << ",\"cpuUtilizationPct\":" << snapshot.cpu_utilization * 100.0
            << ",\"thermalPressure\":" << snapshot.thermal_pressure
            << ",\"ramBytes\":" << snapshot.ram_bytes
            << ",\"zramBytes\":" << snapshot.zram_bytes
            << ",\"refreshHz\":" << snapshot.refresh_hz
            << ",\"gpuModel\":" << json_escape(snapshot.gpu_model)
            << ",\"storageType\":" << json_escape(snapshot.storage_type)
            << ",\"discoveredNodes\":" << snapshot.discovered_nodes
            << ",\"baselineNodes\":" << snapshot.baseline_nodes
            << ",\"verifiedMutations\":" << snapshot.verified_mutations
            << ",\"skippedNodes\":" << snapshot.skipped_nodes
            << ",\"ownershipLosses\":" << snapshot.ownership_losses
            << ",\"journalFault\":" << (snapshot.journal_fault ? "true" : "false")
            << ",\"cpuClusters\":[";

                std::map<std::string, std::vector<NodeValue>> clusters;
        for (const auto& node : manifest.nodes) {
            if (node.domain == NodeDomain::Cpu && !node.advertised_values.empty()) {
                auto& opps = clusters[node.domain_id];
                if (opps.empty()) opps = node.advertised_values;
            }
        }
        bool first = true;
        for (const auto& [cluster_id, opps] : clusters) {
            if (!first) out << ',';
            first = false;
            out << "{\"id\":" << json_escape(cluster_id) << ",\"opps\":[";
            bool first_opp = true;
            for (const auto& candidate : opps) {
                if (!first_opp) out << ',';
                first_opp = false;
                out << json_escape(node_value_text(candidate));
            }
            out << "]}";
        }
        out << "],\"appliedValues\":[";
        const auto audit = baseline.get_audit_trail();
        std::map<std::string, NodeSnapshot> audit_by_path;
        for (const auto& entry : audit) audit_by_path.emplace(entry.path, entry);
        bool first_value = true;
        for (const auto& node : manifest.nodes) {
            const auto found = audit_by_path.find(node.discovered_path.string());
            if (found == audit_by_path.end()) continue;
            const auto& saved = found->second;
            const auto target = last_targets.find(node.id);
            const auto current = read_text(node.discovered_path);
            const bool owned = baseline.owns_expected_state(node.discovered_path.string(),
                saved.is_modified ? saved.active_value : saved.factory_value);
            if (!first_value) out << ',';
            first_value = false;
            out << "{\"semantic\":" << json_escape(node.semantic)
                << ",\"domain\":" << json_escape(node.domain_id)
                << ",\"path\":" << json_escape(node.discovered_path.string())
                << ",\"current\":" << json_escape(current)
                << ",\"oemBaseline\":" << json_escape(saved.factory_value)
                << ",\"target\":" << json_escape(target == last_targets.end() ? saved.active_value : node_value_text(target->second))
                << ",\"modified\":" << (saved.is_modified ? "true" : "false")
                << ",\"owned\":" << (owned ? "true" : "false")
                << ",\"health\":" << json_escape(actuator.health_name(node.id)) << '}';
        }
        out << "]}";
        snapshot.status_json = out.str();
    }

    void start() noexcept {
        // CoreFlowContext::Start has already run BaselineStore recovery. Only
        // after that verified recovery may an interrupted intent WAL be retired.
        if (!clear_direct_adaptive_wal()) {
            snapshot.journal_fault = true;
            snapshot.mode = OperatingMode::Unknown;
            build_json();
            return;
        }
        const auto discovery = scanner.discover({"/sys", "/proc"});
        manifest = discovery.manifest;
        snapshot.discovery_complete = discovery.enumeration_completed;
        snapshot.discovered_nodes = manifest.nodes.size();
        oem = telemetry.capture_oem_baseline(manifest);
        snapshot.baseline_nodes = oem.entries.size();
        snapshot.started = true;

        // Retain a concise model label only when the discovered driver reported one.
        for (const auto& node : manifest.nodes) {
            if (node.domain == NodeDomain::Gpu) {
                snapshot.gpu_model = node.domain_id;
                break;
            }
        }
        for (const auto& node : manifest.nodes) {
            if (node.domain == NodeDomain::Storage) {
                snapshot.storage_type = node.domain_id.substr(0, node.domain_id.find(':'));
                break;
            }
        }
        tick();
    }

    void tick(double thermal_pressure = 0.0, bool thermal_valid = false,
              bool thermal_emergency = false) noexcept {
        if (!snapshot.started) return;
        // Re-read every resolved node before policy evaluation. The manifest is a
        // topology/capability snapshot, never a stale value snapshot. A replaced
        // inode loses write eligibility and is not silently adopted.
        for (auto& node : manifest.nodes) {
            const auto current_text = read_text(node.discovered_path);
            if (current_text.empty()) {
                node.read_probe = ProbeState::Missing;
                node.write_probe = ProbeState::Missing;
                node.access = AccessState::ReadOnly;
                continue;
            }
            const auto live_identity = identity_for(node.discovered_path);
            if (!live_identity.valid() ||
                live_identity.device != node.identity.device ||
                live_identity.inode != node.identity.inode) {
                node.write_probe = ProbeState::IoError;
                node.access = AccessState::ReadOnly;
                continue;
            }
            node.observed_value = parse_node_value(current_text);
            node.access = ::access(node.discovered_path.c_str(), W_OK) == 0
                ? AccessState::Writable : AccessState::ReadOnly;
            node.write_probe = node.access == AccessState::Writable
                ? ProbeState::Available : ProbeState::PermissionDenied;
        }
        bool screen_off = false;
        std::uint64_t ram_bytes = 0U, zram_bytes = 0U;
        std::uint32_t refresh_hz = 0U;
        double cpu_util = 0.0;
        const auto raw = telemetry.sample(manifest, screen_off,
            ram_bytes, zram_bytes, refresh_hz, cpu_util);
        state = telemetry.normalize(manifest, raw);
        const auto now = Clock::now();
        if (thermal_valid) {
            state.metrics.push_back({"thermal.pressure",
                std::clamp(thermal_pressure / 100.0, 0.0, 1.0), 1.0, now, true});
        }
        if (thermal_emergency) state.metrics.push_back({"thermal.emergency", 1.0, 1.0, now, true});

        snapshot.screen_off = screen_off;
        snapshot.ram_bytes = ram_bytes;
        snapshot.zram_bytes = zram_bytes;
        snapshot.refresh_hz = refresh_hz;
        snapshot.cpu_utilization = cpu_util;
        snapshot.thermal_pressure = thermal_valid ? thermal_pressure : 0.0;
        snapshot.thermal_guard = thermal_emergency || (thermal_valid && thermal_pressure >= 85.0);
        snapshot.mode = screen_off ? OperatingMode::Idle :
            snapshot.thermal_guard ? OperatingMode::ThermalGuard :
            cpu_util >= 0.70 ? OperatingMode::HighLoad : OperatingMode::Balanced;

        // Screen-off entry applies an idle envelope once. While the display
        // remains off we avoid repeating policy/discovery work; timerfd status
        // heartbeat continues independently in the daemon EventReactor.
        if (screen_off && last_evaluation != Timestamp{} && snapshot.mode == OperatingMode::Idle) {
            build_json();
            return;
        }
        const auto intent = controller.evaluate(manifest, oem, state, ram_bytes,
            zram_bytes, refresh_hz, snapshot.storage_type, snapshot.mode, now);
        if (intent.mode != OperatingMode::Unknown) snapshot.mode = intent.mode;
        if (!mutation_enabled) {
            last_evaluation = now;
            build_json();
            return;
        }
        if (!intent.actions.empty()) {
            const auto result = actuator.execute(manifest, oem, intent, algorithm);
            if (result.global_mutation_stop || result.journal_status == JournalStatus::JournalFault ||
                result.policy_rejected) {
                snapshot.journal_fault = result.global_mutation_stop ||
                    result.journal_status == JournalStatus::JournalFault;
            }
            snapshot.verified_mutations = 0U;
            snapshot.skipped_nodes = 0U;
            snapshot.ownership_losses = 0U;
            for (const auto& node_result : result.nodes) {
                if (node_result.health == ActuatorHealth::Healthy) ++snapshot.verified_mutations;
                else ++snapshot.skipped_nodes;
                if (node_result.health == ActuatorHealth::IdentityLost ||
                    node_result.health == ActuatorHealth::OemContention) ++snapshot.ownership_losses;
            }
            for (const auto& action : intent.actions) last_targets[action.node_id] = action.target_value;
        }
        last_evaluation = now;
        build_json();
    }
};

DirectAdaptiveRuntime::DirectAdaptiveRuntime(BaselineStore& baseline) noexcept
    : impl_(std::make_unique<Impl>(baseline)) {}

DirectAdaptiveRuntime::~DirectAdaptiveRuntime() = default;

void DirectAdaptiveRuntime::set_mutation_enabled(bool enabled) noexcept {
    if (impl_) impl_->mutation_enabled = enabled;
}

bool DirectAdaptiveRuntime::start() noexcept {
    if (!impl_) return false;
    impl_->start();
    return impl_->snapshot.started;
}

void DirectAdaptiveRuntime::refresh() noexcept {
    if (!impl_) return;
    impl_->snapshot.started = false;
    impl_->start();
}

void DirectAdaptiveRuntime::tick(
    double thermal_pressure, bool thermal_valid, bool thermal_emergency) noexcept {
    if (impl_) impl_->tick(thermal_pressure, thermal_valid, thermal_emergency);
}

const DirectAdaptiveSnapshot& DirectAdaptiveRuntime::snapshot() const noexcept {
    static const DirectAdaptiveSnapshot empty{};
    return impl_ ? impl_->snapshot : empty;
}

std::string DirectAdaptiveRuntime::status_json() const {
    return impl_ ? impl_->snapshot.status_json : "{}";
}

bool DirectAdaptiveRuntime::started() const noexcept {
    return impl_ && impl_->snapshot.started;
}

} // namespace coreflow::runtime
