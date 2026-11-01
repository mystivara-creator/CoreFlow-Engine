#pragma once
#include "coreflow/baseline_intelligence.hpp"
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>

namespace coreflow {
inline const char* outcomeReasonName(BaselineIntelligence::EvaluationReason reason) noexcept {
    using R=BaselineIntelligence::EvaluationReason;
    switch (reason) {
        case R::InsufficientSamples: return "insufficient_samples";
        case R::ContextChanged: return "charging_context_changed";
        case R::RenderingMissing: return "rendering_coverage_lost";
        case R::RefreshRateChanged: return "refresh_period_changed";
        case R::InsufficientEvidence: return "insufficient_comparable_evidence";
        case R::RenderingRegression: return "rendering_regressed";
        case R::ScoreImproved: return "weighted_metrics_improved";
        case R::ScoreRegressed: return "weighted_metrics_regressed";
        case R::WithinNoise: return "within_neutral_band";
    }
    return "unknown";
}
// Fixed keys and numeric values only; this is diagnostic data, never authority.
inline std::string outcomeEvaluationJson(const BaselineIntelligence::Evaluation& e) {
    std::ostringstream out; out.imbue(std::locale::classic()); out << std::setprecision(8);
    const auto number=[&](double v) { if (std::isfinite(v)) out << v; else out << "null"; };
    out << "{\"reason\":\"" << outcomeReasonName(e.reason) << "\",\"outcome\":" << static_cast<int>(e.outcome)
        << ",\"valid\":" << (e.valid ? "true" : "false") << ",\"score\":"; number(e.overall_score);
    out << ",\"confidence\":"; number(e.confidence);
    out << ",\"context_compatible\":" << (e.context_compatible ? "true" : "false")
        << ",\"comparable_metrics\":" << e.comparable_metrics
        << ",\"baseline_samples\":" << e.baseline_samples << ",\"observation_samples\":" << e.observation_samples
        << ",\"metrics\":{";
    constexpr const char* names[]={"thermal_c","load1","cpu_fraction","memory_fraction","psi_fraction","io_latency_ms","jank_fraction"};
    for (std::size_t i=0;i<e.metrics.size();++i) {
        if (i) out << ',';
        const auto& m=e.metrics[i]; out << '"' << names[i] << "\":{\"before\":";
        if (m.before_samples) number(m.before); else out << "null";
        out << ",\"after\":"; if (m.after_samples) number(m.after); else out << "null";
        out << ",\"before_samples\":" << m.before_samples << ",\"after_samples\":" << m.after_samples << ",\"score\":";
        if (m.before_samples>=2U && m.after_samples>=2U) number(m.score); else out << "null";
        out << '}';
    }
    out << "}}"; return out.str();
}
} // namespace coreflow
