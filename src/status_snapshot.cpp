#include "coreflow/status_snapshot.hpp"

#include <cmath>
#include <cstdio>
#include <fcntl.h>
#include <iterator>
#include <locale>
#include <sstream>
#include <unistd.h>

namespace coreflow {
namespace {

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

void appendEscaped(std::ostringstream& out, const std::string& text) {
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
                    static const char* hex = "0123456789abcdef";
                    out << "\\u00" << hex[(c >> 4) & 0xF] << hex[c & 0xF];
                } else {
                    out << raw;
                }
        }
    }
    out << '"';
}

void appendNumber(std::ostringstream& out, double value) {
    if (!std::isfinite(value)) {
        out << "null";
    } else {
        out << value;
    }
}

void appendBool(std::ostringstream& out, bool value) {
    out << (value ? "true" : "false");
}

void appendPairs(std::ostringstream& out,
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
    if (s.mode != "adaptive") return "OBSERVE_MODE";
    if (!s.armed) return "NOT_ARMED";
    if (!s.baseline_ready) return "BASELINE_CAPTURE";
    if (s.observing) return "OBSERVING_OUTCOME";
    if (s.cpu_mutated || s.resource_mutated) return "CHANGE_HELD";
    if (s.cooldown_remaining > 0) return "COOLDOWN";
    if (!s.proactive_allowed && !s.stabilizing_allowed) {
        if (s.state == "IDLE") return "CONTEXT_IDLE";
        if (s.confidence < 0.70) return "CONTEXT_LOW_CONFIDENCE";
        return "CONTEXT_NO_HEADROOM";
    }
    if (!s.mutation_eligible || s.candidates == 0) return "NO_CANDIDATES";
    return "READY";
}

std::string statusToJson(const EngineStatus& s) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out.precision(6);

    out << "{\"schema\":" << kStatusSchemaVersion << ",\"version\":";
    appendEscaped(out, s.version);
    out << ",\"pid\":" << s.pid << ",\"sample\":" << s.sample
        << ",\"updated_epoch\":" << s.updated_epoch
        << ",\"interval_s\":" << s.interval_s << ",\"running\":";
    appendBool(out, s.running);
    out << ",\"blocker\":";
    appendEscaped(out, primaryBlocker(s));

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
    out << ",\"load1\":";
    appendNumber(out, s.load1);
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
        << ",\"baseline_target\":" << s.baseline_target << ",\"observing\":";
    appendBool(out, s.observing);
    out << ",\"cooldown_remaining\":" << s.cooldown_remaining << '}';

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

    return out.str();
}

bool writeStatusFile(const std::string& path, const std::string& json) noexcept {
    try {
        if (path.empty() || json.empty()) return false;
        const std::string tmp = path + ".tmp";
        const int fd = ::open(tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
        if (fd < 0) return false;

        std::size_t written = 0;
        bool ok = true;
        while (written < json.size()) {
            const ssize_t n = ::write(fd, json.data() + written, json.size() - written);
            if (n <= 0) { ok = false; break; }
            written += static_cast<std::size_t>(n);
        }
        if (::close(fd) != 0) ok = false;
        if (!ok || std::rename(tmp.c_str(), path.c_str()) != 0) {
            (void)::unlink(tmp.c_str());
            return false;
        }
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace coreflow
