#include "coreflow/experience.hpp"

#include <algorithm>
#include <cmath>

namespace coreflow {

bool ExperienceMemory::Context::compatibleWith(
    const Context& other) const noexcept {
    if (state != other.state) return false;
    if (charging != other.charging) return false;

    if (thermal_trend != Trend::Unknown &&
        other.thermal_trend != Trend::Unknown &&
        thermal_trend != other.thermal_trend) {
        return false;
    }

    if (memory_trend != Trend::Unknown &&
        other.memory_trend != Trend::Unknown &&
        memory_trend != other.memory_trend) {
        return false;
    }

    if (load_trend != Trend::Unknown &&
        other.load_trend != Trend::Unknown &&
        load_trend != other.load_trend) {
        return false;
    }

    return true;
}

bool ExperienceMemory::sameCandidate(
    const CandidateIdentity& lhs,
    const CandidateIdentity& rhs) noexcept {
    return lhs.valid() && rhs.valid() && lhs.key == rhs.key;
}

bool ExperienceMemory::finite(double value) noexcept {
    return std::isfinite(value);
}

double ExperienceMemory::clamp01(double value) noexcept {
    if (!finite(value)) return 0.0;
    return std::clamp(value, 0.0, 1.0);
}

bool ExperienceMemory::record(const Record& input) noexcept {
    if (!input.candidate.valid() ||
        !finite(input.score) ||
        !finite(input.confidence)) {
        return false;
    }

    Record normalized = input;
    normalized.score = clamp01(normalized.score);
    normalized.confidence = clamp01(normalized.confidence);

    for (Record& existing : records_) {
        if (!sameCandidate(existing.candidate, normalized.candidate) ||
            !existing.context.compatibleWith(normalized.context)) {
            continue;
        }

        const double oldWeight = static_cast<double>(existing.observations);
        const double newWeight = static_cast<double>(
            std::max<std::uint64_t>(normalized.observations, 1U));
        const double totalWeight = oldWeight + newWeight;

        if (totalWeight > 0.0) {
            existing.score =
                ((existing.score * oldWeight) +
                 (normalized.score * newWeight)) / totalWeight;
            existing.confidence =
                ((existing.confidence * oldWeight) +
                 (normalized.confidence * newWeight)) / totalWeight;
        } else {
            existing.score = normalized.score;
            existing.confidence = normalized.confidence;
        }

        existing.outcome = normalized.outcome;
        existing.observations += normalized.observations;
        return true;
    }

    records_.push_back(normalized);
    return true;
}

bool ExperienceMemory::recordEvaluation(
    const CandidateIdentity& candidate,
    const Context& context,
    const BaselineIntelligence::Evaluation& evaluation) noexcept {
    if (!candidate.valid() ||
        !evaluation.valid ||
        !std::isfinite(evaluation.overall_score) ||
        !std::isfinite(evaluation.confidence)) {
        return false;
    }

    Record record;
    record.candidate = candidate;
    record.context = context;
    record.outcome = evaluation.outcome;
    record.score = evaluation.overall_score;
    record.confidence = evaluation.confidence;
    record.observations = evaluation.observation_samples;

    return this->record(record);
}

const ExperienceMemory::Record* ExperienceMemory::find(
    const CandidateIdentity& candidate,
    const Context& context) const noexcept {
    if (!candidate.valid()) return nullptr;

    const Record* best = nullptr;

    for (const Record& record : records_) {
        if (!sameCandidate(record.candidate, candidate) ||
            !record.context.compatibleWith(context)) {
            continue;
        }

        if (best == nullptr ||
            record.confidence > best->confidence ||
            (record.confidence == best->confidence &&
             record.observations > best->observations)) {
            best = &record;
        }
    }

    return best;
}

} // namespace coreflow
