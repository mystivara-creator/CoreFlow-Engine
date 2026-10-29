#include "coreflow/status_snapshot.hpp"

#include <array>
#include <cerrno>
#include <charconv>
#include <cmath>
#include <concepts>
#include <cstdio>
#include <fcntl.h>
#include <iterator>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <type_traits>
#include <unistd.h>
#include <utility>

namespace coreflow {
namespace {

constexpr const char* kConsultReasons[] = {
    "NO_WRITE_ACTUATOR", "OUT_OF_SCOPE", "SURFACE_DENIED", "NOT_OBSERVABLE",
    "NO_ROLLBACK_BASELINE", "NO_SEMANTIC_PRIOR", "STRESSED_NON_STABILIZING",
    "NO_NEED", "NO_BENEFIT_EVIDENCE", "AWAITING_USER_GRANT", "ALLOWED"
};
constexpr const char* kConsultNamespaces[] = {
    "VM_SYSCTL", "BLOCK_QUEUE", "SCHEDULER", "GPU", "CGROUP", "CPUFREQ_TUNABLE",
    "KERNEL_TUNABLE", "CPU_FREQ", "BUILD_PROP", "SETTINGS_DB", "OTHER"
};
// Must match ControlNamespace exactly; a mismatch reads past the array.
static_assert(sizeof(kConsultNamespaces) / sizeof(kConsultNamespaces[0]) == kControlNamespaceCount,
              "kConsultNamespaces out of sync with ControlNamespace");
static_assert(sizeof(kConsultReasons) / sizeof(kConsultReasons[0]) == kConsultReasonCount,
              "kConsultReasons out of sync with ConsultReason");

// Keep this list in sync with the WebUI (engine_status.js); a host check compares them.
constexpr const char* kBlockerCodes[] = {
    "STOPPED",
    "SAFETY_HOLD",
    "OBSERVE_MODE",
    "NOT_ARMED",
    "BASELINE_CAPTURE",
    "OBSERVING_OUTCOME",
    "CHANGE_HELD",
    "COOLDOWN",
    "CONTEXT_IDLE",
    "CONTEXT_LOW_CONFIDENCE",
    "CONTEXT_NO_HEADROOM",
    "NO_CANDIDATES",
    "READY",
};

class JsonBuffer final {
public:
    explicit JsonBuffer(std::size_t reserve_bytes = 3072U) {
        buf_.reserve(reserve_bytes);
    }

    JsonBuffer& operator<<(char ch) {
        buf_.push_back(ch);
        return *this;
    }

    JsonBuffer& operator<<(const char* text) {
        if (text != nullptr) buf_.append(text);
        return *this;
    }

    JsonBuffer& operator<<(std::string_view text) {
        buf_.append(text);
        return *this;
    }

    JsonBuffer& operator<<(const std::string& text) {
        buf_.append(text);
        return *this;
    }

    template <typename T>
        requires std::integral<T> &&
                 (!std::same_as<std::remove_cv_t<T>, char>) &&
                 (!std::same_as<std::remove_cv_t<T>, bool>)
    JsonBuffer& operator<<(T value) {
        std::array<char, 32> tmp{};
        const auto res = std::to_chars(tmp.data(), tmp.data() + tmp.size(), value);
        if (res.ec == std::errc{}) {
            buf_.append(tmp.data(), static_cast<std::size_t>(res.ptr - tmp.data()));
        } else {
            buf_.push_back('0');
        }
        return *this;
    }

    void appendDouble(double value) {
        if (!std::isfinite(value)) {
            buf_.append("null", 4U);
            return;
        }
        std::array<char, 48> tmp{};
        const auto res = std::to_chars(
            tmp.data(), tmp.data() + tmp.size(), value, std::chars_format::general, 6);
        if (res.ec == std::errc{}) {
            buf_.append(tmp.data(), static_cast<std::size_t>(res.ptr - tmp.data()));
        } else {
            buf_.append("null", 4U);
        }
    }

    std::string take() noexcept { return std::move(buf_); }

private:
    std::string buf_;
};

void appendEscaped(JsonBuffer& out, std::string_view text) {
    constexpr std::size_t kMaxString = 512;
    out << '"';
    std::size_t count = 0;
    for (const char raw : text) {
        if (++count > kMaxString) break;
        const unsigned char c = static_cast<unsigned char>(raw);
        switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (c < 0x20) {
                    static constexpr char hex[] = "0123456789abcdef";
                    out << "\\u00" << hex[(c >> 4) & 0xF] << hex[c & 0xF];
                } else {
                    out << raw;
                }
        }
    }
    out << '"';
}

void appendNumber(JsonBuffer& out, double value) {
    out.appendDouble(value);
}

void appendBool(JsonBuffer& out, bool value) {
    out << (value ? "true" : "false");
}

void appendPairs(JsonBuffer& out,
                 const std::vector<std::pair<std::string, std::string>>& pairs,
                 const char* key_name) {
    constexpr std::size_t kMaxItems = 16;
    out << '[';
    std::size_t index = 0;
    for (const auto& item : pairs) {
        if (index >= kMaxItems) break;
        if (index != 0) out << ',';
        out << "{\"" << key_name << "\":";
        appendEscaped(out, item.first);
        out << ",\"value\":";
        appendEscaped(out, item.second);
        out << '}';
        ++index;
    }
    out << ']';
}

} // namespace

const std::vector<std::string>& blockerCodes() {
    static const std::vector<std::string> codes(
        std::begin(kBlockerCodes), std::end(kBlockerCodes));
    return codes;
}

std::string primaryBlocker(const EngineStatus& s) {
    if (!s.running) return "STOPPED";
    if (s.hold_active) return "SAFETY_HOLD";
    // status.json / WebUI use config-style tokens: "adaptive" | "observe".
    // Logs historically used mutationModeName() → "ADAPTIVE"/"DISABLED".
    // Accept either form so Adaptive is never misreported as OBSERVE_MODE.
    std::string modeLower = s.mode;
    for (char& c : modeLower) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    const bool adaptive = (modeLower == "adaptive");
    if (!adaptive) return "OBSERVE_MODE";
    if (!s.armed) return "NOT_ARMED";
    if (!s.baseline_ready) return "BASELINE_CAPTURE";
    if (s.observing) return "OBSERVING_OUTCOME";
    if (s.cpu_mutated || s.resource_mutated) return "CHANGE_HELD";
    if (s.cooldown_remaining > 0) return "COOLDOWN";
    if (!s.proactive_allowed && !s.stabilizing_allowed) {
        if (s.state == "IDLE") return "CONTEXT_IDLE";
        // Align with EngineConfig min_confidence default (Balance pass: 0.60).
        if (s.confidence < 0.60) return "CONTEXT_LOW_CONFIDENCE";
        return "CONTEXT_NO_HEADROOM";
    }
    if (!s.mutation_eligible || s.candidates == 0) return "NO_CANDIDATES";
    return "READY";
}

std::string statusToJson(const EngineStatus& s) {
    JsonBuffer out(3072U);

    out << "{\"schema\":" << kStatusSchemaVersion << ",\"version\":";
    appendEscaped(out, s.version);
    out << ",\"pid\":" << s.pid << ",\"sample\":" << s.sample
        << ",\"updated_epoch\":" << s.updated_epoch
        << ",\"interval_s\":" << s.interval_s << ",\"running\":";
    appendBool(out, s.running);
    out << ",\"blocker\":";
    appendEscaped(out, primaryBlocker(s));

    out << ",\"intelligence\":{\"pressure\":";
    appendEscaped(out, s.runtime_pressure);
    out << ",\"confidence\":"; appendNumber(out, s.diagnosis_confidence);
    out << ",\"rendering_available\":"; appendBool(out, s.rendering_available);
    out << ",\"frame_jank_rate\":"; appendNumber(out, s.frame_jank_rate);
    out << ",\"control_conflict\":"; appendBool(out, s.control_conflict);
    out << ",\"learned_candidates\":" << s.learned_candidates;
    out << ",\"storage_available\":"; appendBool(out, s.storage_health.available);
    out << ",\"storage_caution\":"; appendBool(out, s.storage_health.caution);
    out << ",\"ufs_life_a\":" << s.storage_health.life_a
        << ",\"ufs_life_b\":" << s.storage_health.life_b
        << ",\"ufs_pre_eol\":" << s.storage_health.pre_eol
        << ",\"power_hal_backend\":\"vendor_profiles_and_continuity\""
        << ",\"adpf_backend\":\"application_integration_required\"}";

    out << ",\"device\":{\"manufacturer\":";
    appendEscaped(out, s.manufacturer);
    out << ",\"model\":"; appendEscaped(out, s.model);
    out << ",\"device_name\":"; appendEscaped(out, s.device);
    out << ",\"product\":"; appendEscaped(out, s.product);
    out << ",\"board\":"; appendEscaped(out, s.board);
    out << ",\"hardware\":"; appendEscaped(out, s.hardware);
    out << ",\"soc_manufacturer\":"; appendEscaped(out, s.soc_manufacturer);
    out << ",\"soc_model\":"; appendEscaped(out, s.soc_model);
    out << ",\"android_release\":"; appendEscaped(out, s.android_release);
    out << ",\"sdk_level\":"; appendEscaped(out, s.sdk_level);
    out << ",\"kernel_release\":"; appendEscaped(out, s.kernel_release);
    out << ",\"abi\":"; appendEscaped(out, s.abi);
    out << ",\"proc_available\":"; appendBool(out, s.proc_available);
    out << ",\"sys_available\":"; appendBool(out, s.sys_available);
    out << ",\"cgroup_v2\":"; appendBool(out, s.cgroup_v2);
    out << ",\"cpuset_available\":"; appendBool(out, s.cpuset_available);
    out << ",\"uclamp_available\":"; appendBool(out, s.uclamp_available);
    out << ",\"scheduler_controls_available\":"; appendBool(out, s.scheduler_controls_available);
    out << ",\"devfreq_available\":"; appendBool(out, s.devfreq_available);
    out << ",\"discovered_resources\":" << s.discovered_resources;
    out << ",\"mutation_ready_resources\":" << s.mutation_ready_resources;
    out << ",\"expanded_domains\":[";
    for (std::size_t i = 0; i < s.expanded_domains.size(); ++i) {
        if (i != 0U) out << ',';
        const auto& domain = s.expanded_domains[i];
        const char* evidence = "unknown";
        switch (domain.evidence) {
            case CapabilityEvidence::Unavailable: evidence = "unavailable"; break;
            case CapabilityEvidence::Detected: evidence = "detected"; break;
            case CapabilityEvidence::Observable: evidence = "observable"; break;
            default: break;
        }
        out << "{\"id\":" << static_cast<unsigned int>(domain.id)
            << ",\"evidence\":\"" << evidence
            << "\",\"matched\":" << domain.matched_resources
            << ",\"readable\":" << domain.readable_resources
            << ",\"mutation_authorized\":"
            << (domain.mutation_authorized ? "true" : "false") << '}';
    }
    out << ']';
    out << ",\"block_queue_mutations_quarantined\":";
    appendBool(out, s.block_queue_mutations_quarantined);
    out << "},\"eyes\":{\"thermal_available\":";
    appendBool(out, s.thermal_available);
    out << ",\"thermal_c\":";
    if (s.thermal_available) appendNumber(out, s.thermal_c); else out << "null";
    out << ",\"hottest_c\":";
    if (s.thermal_available) appendNumber(out, s.hottest_c); else out << "null";
    out << ",\"thermal_trend\":";
    appendEscaped(out, s.thermal_trend);
    out << ",\"mem_available_ratio\":";
    appendNumber(out, s.mem_available_ratio);
    out << ",\"memory_trend\":";
    appendEscaped(out, s.memory_trend);
    out << ",\"cpu_utilization\":";
    if (s.cpu_available) appendNumber(out, s.cpu_utilization); else out << "null";
    out << ",\"load_available\":";
    appendBool(out, s.load_available);
    out << ",\"load1\":";
    if (s.load_available) appendNumber(out, s.load1); else out << "null";
    out << ",\"load_trend\":";
    appendEscaped(out, s.load_trend);
    out << ",\"io_read_kbs\":";
    if (s.io_available) appendNumber(out, s.io_read_kbs); else out << "null";
    out << ",\"io_write_kbs\":";
    if (s.io_available) appendNumber(out, s.io_write_kbs); else out << "null";
    out << ",\"battery_percent\":";
    if (s.battery_percent >= 0) out << s.battery_percent; else out << "null";
    out << ",\"charging\":";
    appendBool(out, s.charging);
    out << ",\"confidence\":";
    appendNumber(out, s.confidence);
    out << '}';

    out << ",\"brain\":{\"state\":";
    appendEscaped(out, s.state);
    out << ",\"previous_state\":";
    appendEscaped(out, s.previous_state);
    out << ",\"decision\":";
    appendEscaped(out, s.decision);
    out << ",\"workload\":";
    appendEscaped(out, s.workload);
    out << ",\"thermal_headroom\":";
    appendBool(out, s.thermal_headroom);
    out << ",\"memory_headroom\":";
    appendBool(out, s.memory_headroom);
    out << ",\"power_headroom\":";
    appendBool(out, s.power_headroom);
    out << ",\"proactive_allowed\":";
    appendBool(out, s.proactive_allowed);
    out << ",\"stabilizing_allowed\":";
    appendBool(out, s.stabilizing_allowed);
    out << ",\"plan_action\":";
    appendEscaped(out, s.plan_action);
    out << ",\"intervention\":";
    appendEscaped(out, s.intervention);
    out << ",\"mutation_eligible\":";
    appendBool(out, s.mutation_eligible);
    out << ",\"stabilizing_only\":";
    appendBool(out, s.stabilizing_only);
    out << ",\"candidates\":" << s.candidates << ",\"plan_reason\":";
    appendEscaped(out, s.plan_reason);
    out << ",\"agent_mode\":";
    appendEscaped(out, s.agent_mode);
    out << ",\"agent_reason\":";
    appendEscaped(out, s.agent_reason);
    out << ",\"hold_active\":";
    appendBool(out, s.hold_active);
    out << ",\"safety_hold\":";
    appendEscaped(out, s.safety_hold);
    out << ",\"mode\":";
    appendEscaped(out, s.mode);
    out << ",\"armed\":";
    appendBool(out, s.armed);
    out << ",\"allow_cpu_governor\":";
    appendBool(out, s.allow_cpu_governor);
    out << ",\"permit_resource\":";
    appendBool(out, s.permit_resource);
    out << ",\"permit_cpu\":";
    appendBool(out, s.permit_cpu);
    out << ",\"baseline_ready\":";
    appendBool(out, s.baseline_ready);
    out << ",\"baseline_samples\":" << s.baseline_samples
        << ",\"baseline_target\":" << s.baseline_target
        << ",\"observation_samples\":" << s.observation_samples
        << ",\"observation_target\":" << s.observation_target
        << ",\"skip_reason\":";
    appendEscaped(out, s.skip_reason);
    out << ",\"observing\":";
    appendBool(out, s.observing);
    out << ",\"cooldown_remaining\":" << s.cooldown_remaining;
    out << ",\"consultation\":{\"by_reason\":{";
    for (std::size_t i = 0; i < s.consultation_ledger.by_reason.size(); ++i) {
        if (i != 0U) out << ',';
        out << '"' << kConsultReasons[i] << "\":"
            << s.consultation_ledger.by_reason[i];
    }
    out << "},\"by_namespace\":{";
    for (std::size_t i = 0; i < s.consultation_ledger.last_reason.size(); ++i) {
        if (i != 0U) out << ',';
        out << '"' << kConsultNamespaces[i]
            << "\":{\"last_reason\":";
        const auto reason_index = static_cast<std::size_t>(s.consultation_ledger.last_reason[i]);
        appendEscaped(out, reason_index < (sizeof(kConsultReasons) / sizeof(kConsultReasons[0]))
                               ? kConsultReasons[reason_index] : "UNKNOWN");
        out << ",\"need_present\":";
        appendBool(out, s.consultation_ledger.last_need[i]);
        out << '}';
    }
    out << "}}";
    out << '}';

    out << ",\"hands\":{\"cpu_mutated\":";
    appendBool(out, s.cpu_mutated);
    out << ",\"resource_mutated\":";
    appendBool(out, s.resource_mutated);
    out << ",\"stabilizing_epoch\":";
    appendBool(out, s.stabilizing_epoch);
    out << ",\"cpu_applied\":";
    appendPairs(out, s.cpu_applied, "key");
    out << ",\"resource_applied\":";
    appendPairs(out, s.resource_applied, "path");
    out << ",\"last_cpu_result\":";
    appendEscaped(out, s.last_cpu_result);
    out << ",\"last_resource_result\":";
    appendEscaped(out, s.last_resource_result);
    out << ",\"last_change_kind\":";
    appendEscaped(out, s.last_change_kind);
    out << ",\"last_change_result\":";
    appendEscaped(out, s.last_change_result);
    out << ",\"last_change_sample\":" << s.last_change_sample
        << ",\"verified\":" << s.verified
        << ",\"rolled_back\":" << s.rolled_back
        << ",\"failed\":" << s.failed << "}}";

    return out.take();
}

bool writeStatusFile(const std::string& path, const std::string& json) noexcept {
    try {
        if (path.empty() || json.empty() || path.find('\0') != std::string::npos) {
            return false;
        }
        // Unique exclusive temp (same pattern as the mutation journal): a fixed
        // "*.tmp" path is vulnerable to hard-link TOCTOU even with O_NOFOLLOW.
        std::string temporary = path + ".tmp.XXXXXX";
        const int fd = ::mkstemp(temporary.data());
        if (fd < 0) return false;

        struct stat st {};
        bool ok = ::fcntl(fd, F_SETFD, FD_CLOEXEC) == 0 &&
                  ::fstat(fd, &st) == 0 && S_ISREG(st.st_mode) &&
                  ::fchmod(fd, 0600) == 0;

        std::size_t written = 0;
        while (ok && written < json.size()) {
            const ssize_t n = ::write(fd, json.data() + written, json.size() - written);
            if (n < 0) {
                if (errno == EINTR) continue;
                ok = false;
                break;
            }
            if (n == 0) {
                ok = false;
                break;
            }
            written += static_cast<std::size_t>(n);
        }
        if (::close(fd) != 0) ok = false;
        if (!ok || std::rename(temporary.c_str(), path.c_str()) != 0) {
            (void)::unlink(temporary.c_str());
            return false;
        }
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace coreflow
