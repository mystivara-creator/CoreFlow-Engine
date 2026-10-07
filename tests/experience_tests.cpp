#include "coreflow/experience.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

using coreflow::BaselineIntelligence;
using coreflow::ExperienceMemory;
using coreflow::RuntimeState;
using coreflow::Trend;

namespace {

ExperienceMemory::Context normalContext() {
    ExperienceMemory::Context context;
    context.state = RuntimeState::Normal;
    context.charging = false;
    context.thermal_trend = Trend::Rising;
    context.memory_trend = Trend::Stable;
    context.load_trend = Trend::Rising;
    return context;
}

ExperienceMemory::Record makeRecord(
    const char* key,
    double score,
    double confidence,
    std::uint64_t observations) {
    ExperienceMemory::Record record;
    record.candidate.key = key;
    record.context = normalContext();
    record.outcome = BaselineIntelligence::Outcome::Beneficial;
    record.score = score;
    record.confidence = confidence;
    record.observations = observations;
    return record;
}

} // namespace

int main() {
    {
        ExperienceMemory memory;
        const auto context = normalContext();
        const auto record = makeRecord("policy0:performance", 0.80, 0.90, 3U);

        assert(memory.size() == 0U);
        assert(memory.record(record));
        assert(memory.size() == 1U);

        ExperienceMemory::CandidateIdentity candidate;
        candidate.key = "policy0:performance";

        const auto* found = memory.find(candidate, context);
        assert(found != nullptr);
        assert(found->candidate.key == candidate.key);
        assert(std::abs(found->score - 0.80) < 1e-12);
        assert(std::abs(found->confidence - 0.90) < 1e-12);
        assert(found->observations == 3U);
    }

    {
        ExperienceMemory memory;
        assert(memory.record(makeRecord(
            "policy0:performance", 0.80, 0.80, 2U)));
        assert(memory.record(makeRecord(
            "policy0:performance", 0.60, 0.60, 2U)));

        ExperienceMemory::CandidateIdentity candidate;
        candidate.key = "policy0:performance";

        const auto* found = memory.find(candidate, normalContext());
        assert(found != nullptr);

        // Weighted merge: (0.80*2 + 0.60*2) / 4 = 0.70.
        assert(std::abs(found->score - 0.70) < 1e-12);
        assert(std::abs(found->confidence - 0.70) < 1e-12);
        assert(found->observations == 4U);
    }

    {
        ExperienceMemory memory;
        assert(memory.record(makeRecord(
            "policy0:performance", 0.90, 0.90, 3U)));

        ExperienceMemory::CandidateIdentity candidate;
        candidate.key = "policy0:performance";

        auto incompatible = normalContext();
        incompatible.charging = true;

        assert(memory.find(candidate, incompatible) == nullptr);
    }

    {
        ExperienceMemory memory;

        BaselineIntelligence::Evaluation evaluation;
        evaluation.valid = true;
        evaluation.outcome = BaselineIntelligence::Outcome::Beneficial;
        evaluation.overall_score = 0.75;
        evaluation.confidence = 0.85;
        evaluation.observation_samples = 4U;

        ExperienceMemory::CandidateIdentity candidate;
        candidate.key = "policy1:balanced";

        assert(memory.recordEvaluation(
            candidate, normalContext(), evaluation));

        const auto* found = memory.find(candidate, normalContext());
        assert(found != nullptr);
        assert(found->outcome == BaselineIntelligence::Outcome::Beneficial);
        assert(std::abs(found->score - 0.75) < 1e-12);
        assert(std::abs(found->confidence - 0.85) < 1e-12);
        assert(found->observations == 4U);
    }

    {
        ExperienceMemory memory;
        ExperienceMemory::CandidateIdentity candidate;
        candidate.key = "policy0:performance";

        assert(memory.record(makeRecord(
            "policy0:performance", 1.50, 2.00, 1U)));

        const auto* found = memory.find(candidate, normalContext());
        assert(found != nullptr);
        assert(found->score == 1.0);
        assert(found->confidence == 1.0);
    }

    std::cout << "experience_tests: PASS\n";
    return 0;
}
