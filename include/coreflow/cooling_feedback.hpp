#pragma once
#include "thermal_monitor.hpp"
#include <cmath>
#include <optional>
#include <string>
#include <unordered_map>

namespace coreflow {
// Observes change during mitigation; does not claim that CoreFlow caused cooling.
class CoolingFeedback {
public:
    std::optional<std::string> sample(const ThermalAssessment& assessment, double now, bool mitigating) {
        if (!assessment.available || !mitigating || !std::isfinite(now)) { reset(); return {}; }
        if (!started_) { capture(assessment, now); return {}; }
        if (now - start_ < 10) return {};
        size_t rising = 0, falling = 0, matched = 0;
        for (const auto& sensor : assessment.readings) {
            const auto previous = temperatures_.find(sensor.id);
            if (!sensor.valid || previous == temperatures_.end()) continue;
            ++matched;
            const float change = sensor.celsius - previous->second;
            if (change > 0.3f) ++rising;
            else if (change < -0.3f) ++falling;
        }
        const auto severity_before = severity_;
        capture(assessment, now);
        if (matched) {
            if (rising && falling) return "mixed";
            if (rising) return "rising";
            if (falling) return "cooling";
            return "steady";
        }
        if (assessment.severity >= 0 && severity_before >= 0) {
            if (assessment.severity > severity_before) return "OEM severity rising";
            if (assessment.severity < severity_before) return "OEM severity falling";
            return "OEM severity steady; sensor trend unavailable";
        }
        return "unavailable";
    }
    void reset() { started_ = false; temperatures_.clear(); severity_ = -1; }
private:
    void capture(const ThermalAssessment& assessment, double now) {
        temperatures_.clear();
        for (const auto& sensor : assessment.readings) if (sensor.valid) temperatures_[sensor.id] = sensor.celsius;
        start_ = now; started_ = true; severity_ = assessment.severity;
    }
    bool started_{false};
    double start_{0};
    int severity_{-1};
    std::unordered_map<std::string, float> temperatures_;
};
}
