#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "coreflow/baseline_intelligence.hpp"
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

    std::vector<Record> records_;
};

} // namespace coreflow
