#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <utility>

#include "coreflow/baseline_intelligence.hpp"
#include "coreflow/context.hpp"
#include "coreflow/types.hpp"

namespace coreflow {

/**
 * Lightweight in-memory experience store for verified runtime outcomes.
 *
 * Experience is advisory only. A historical record never replaces a fresh
 * observation window or the safety decisions owned by the runtime engine.
 */
class ExperienceMemory {
public:
    struct CandidateIdentity {
        std::string key;

        bool valid() const noexcept {
            return !key.empty();
        }
    };

    struct Context {
        RuntimeState state{RuntimeState::Idle};
        WorkloadClass workload{WorkloadClass::Unknown};
        bool charging{false};
        Trend thermal_trend{Trend::Unknown};
        Trend memory_trend{Trend::Unknown};
        Trend load_trend{Trend::Unknown};

        bool compatibleWith(const Context& other) const noexcept;
    };

    struct Record {
        CandidateIdentity candidate;
        Context context;
        BaselineIntelligence::Outcome outcome{BaselineIntelligence::Outcome::Unknown};
        double score{0.0};
        double confidence{0.0};
        std::uint64_t observations{0};
    };

    bool record(const Record& record) noexcept;

    // Persistence is deliberately bounded and advisory. The store is written
    // atomically and is never consulted as a safety authority.
    void setScope(std::string scope) noexcept { scope_ = std::move(scope); }
    bool load(const std::string& path) noexcept;
    bool flush(const std::string& path) noexcept;
    bool dirty() const noexcept { return dirty_; }

    bool recordEvaluation(
        const CandidateIdentity& candidate,
        const Context& context,
        const BaselineIntelligence::Evaluation& evaluation) noexcept;


    const Record* find(const CandidateIdentity& candidate,
                       const Context& context) const noexcept;

    std::size_t size() const noexcept { return records_.size(); }
    void clear() noexcept { records_.clear(); }

private:
    static bool sameCandidate(const CandidateIdentity& lhs,
                              const CandidateIdentity& rhs) noexcept;
    static bool finite(double value) noexcept;
    static double clamp01(double value) noexcept;

    static constexpr std::size_t kMaxRecords = 128;
    static bool safeToken(const std::string& value) noexcept;
    static int enumValue(RuntimeState value) noexcept;
    static int enumValue(Trend value) noexcept;
    static RuntimeState runtimeStateFromInt(int value) noexcept;
    static Trend trendFromInt(int value) noexcept;

    std::string scope_;
    std::vector<Record> records_;
    bool dirty_{false};
};


// Blend a candidate's prior score with its measured outcome.
// Both scores use [0,1], where 0.5 represents a neutral outcome.
// Unknown/inconclusive or low-confidence history never changes the prior.
inline double blendExperienceScore(
    double prior_score,
    const ExperienceMemory::Record* record) noexcept {
    if (!std::isfinite(prior_score)) return 0.0;
    prior_score = std::clamp(prior_score, 0.0, 1.0);

    if (record == nullptr || record->observations == 0 ||
        !std::isfinite(record->score) ||
        !std::isfinite(record->confidence) ||
        record->confidence < 0.60 ||
        record->score < 0.0 || record->score > 1.0) {
        return prior_score;
    }

    switch (record->outcome) {
        case BaselineIntelligence::Outcome::Beneficial:
        case BaselineIntelligence::Outcome::Neutral:
        case BaselineIntelligence::Outcome::Regression:
            break;
        case BaselineIntelligence::Outcome::Unknown:
        case BaselineIntelligence::Outcome::Inconclusive:
            return prior_score;
    }

    const double confidence =
        std::clamp(record->confidence, 0.0, 1.0);
    return std::clamp(
        prior_score + (record->score - prior_score) * confidence,
        0.0, 1.0);
}

} // namespace coreflow
