#include "coreflow/experience.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>

namespace coreflow {

bool ExperienceMemory::Context::compatibleWith(
    const Context& other) const noexcept {
    if (state != other.state) return false;
    if (workload != WorkloadClass::Unknown &&
        other.workload != WorkloadClass::Unknown &&
        workload != other.workload) return false;
    if (charging != other.charging) return false;

    if (thermal_trend != Trend::Unknown &&
        other.thermal_trend != Trend::Unknown &&
        thermal_trend != other.thermal_trend) return false;
    if (memory_trend != Trend::Unknown &&
        other.memory_trend != Trend::Unknown &&
        memory_trend != other.memory_trend) return false;
    if (load_trend != Trend::Unknown &&
        other.load_trend != Trend::Unknown &&
        load_trend != other.load_trend) return false;
    return true;
}

bool ExperienceMemory::sameCandidate(
    const CandidateIdentity& lhs,
    const CandidateIdentity& rhs) noexcept {
    return lhs.valid() && rhs.valid() && lhs.key == rhs.key;
}

bool ExperienceMemory::finite(double value) noexcept { return std::isfinite(value); }

double ExperienceMemory::clamp01(double value) noexcept {
    if (!finite(value)) return 0.0;
    return std::clamp(value, 0.0, 1.0);
}

bool ExperienceMemory::safeToken(const std::string& value) noexcept {
    return !value.empty() && value.find_first_of("\t\r\n") == std::string::npos;
}

int ExperienceMemory::enumValue(RuntimeState value) noexcept {
    return static_cast<int>(value);
}

int ExperienceMemory::enumValue(Trend value) noexcept {
    return static_cast<int>(value);
}

RuntimeState ExperienceMemory::runtimeStateFromInt(int value) noexcept {
    if (value < enumValue(RuntimeState::Idle) || value > enumValue(RuntimeState::ThermalGuard))
        return RuntimeState::Idle;
    return static_cast<RuntimeState>(value);
}

Trend ExperienceMemory::trendFromInt(int value) noexcept {
    if (value < enumValue(Trend::Unknown) || value > enumValue(Trend::Falling))
        return Trend::Unknown;
    return static_cast<Trend>(value);
}

bool ExperienceMemory::record(const Record& input) noexcept {
    if (!input.candidate.valid() || !safeToken(input.candidate.key) ||
        !finite(input.score) || !finite(input.confidence)) return false;

    Record normalized = input;
    normalized.score = clamp01(normalized.score);
    normalized.confidence = clamp01(normalized.confidence);
    normalized.observations = std::max<std::uint64_t>(normalized.observations, 1U);

    for (Record& existing : records_) {
        if (!sameCandidate(existing.candidate, normalized.candidate) ||
            !existing.context.compatibleWith(normalized.context)) continue;

        const double oldWeight = static_cast<double>(existing.observations);
        const double newWeight = static_cast<double>(normalized.observations);
        const double totalWeight = oldWeight + newWeight;
        existing.score = ((existing.score * oldWeight) +
                          (normalized.score * newWeight)) / totalWeight;
        existing.confidence = ((existing.confidence * oldWeight) +
                               (normalized.confidence * newWeight)) / totalWeight;
        existing.outcome = normalized.outcome;
        existing.observations += normalized.observations;
        dirty_ = true;
        return true;
    }

    records_.push_back(normalized);
    if (records_.size() > kMaxRecords) {
        std::stable_sort(records_.begin(), records_.end(),
                         [](const Record& a, const Record& b) {
                             if (a.confidence != b.confidence)
                                 return a.confidence > b.confidence;
                             return a.observations > b.observations;
                         });
        records_.resize(kMaxRecords);
    }
    dirty_ = true;
    return true;
}

bool ExperienceMemory::recordEvaluation(
    const CandidateIdentity& candidate,
    const Context& context,
    const BaselineIntelligence::Evaluation& evaluation) noexcept {
    if (!candidate.valid() || !evaluation.valid ||
        !std::isfinite(evaluation.overall_score) ||
        !std::isfinite(evaluation.confidence)) return false;

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
            !record.context.compatibleWith(context)) continue;
        if (best == nullptr || record.confidence > best->confidence ||
            (record.confidence == best->confidence &&
             record.observations > best->observations)) best = &record;
    }
    return best;
}

bool ExperienceMemory::load(const std::string& path) noexcept {
    try {
        std::ifstream file(path);
        if (!file) return false;
        std::string line;
        if (!std::getline(file, line) || line != "COREFLOW_EXPERIENCE_V2") return false;

        std::vector<Record> loaded;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            std::istringstream in(line);
            std::string scope, key, state, workload, charging, thermal, memory, load;
            std::string outcome, score, confidence, observations;
            if (!std::getline(in, scope, '\t') ||
                !std::getline(in, key, '\t') ||
                !std::getline(in, state, '\t') ||
                !std::getline(in, workload, '\t') ||
                !std::getline(in, charging, '\t') ||
                !std::getline(in, thermal, '\t') ||
                !std::getline(in, memory, '\t') ||
                !std::getline(in, load, '\t') ||
                !std::getline(in, outcome, '\t') ||
                !std::getline(in, score, '\t') ||
                !std::getline(in, confidence, '\t') ||
                !std::getline(in, observations)) continue;
            if (!scope_.empty() && scope != scope_) continue;
            if (!safeToken(key)) continue;

            Record r;
            r.candidate.key = key;
            r.context.state = runtimeStateFromInt(std::stoi(state));
            const int workloadValue = std::stoi(workload);
            r.context.workload = workloadValue >= static_cast<int>(WorkloadClass::Unknown) &&
                                  workloadValue <= static_cast<int>(WorkloadClass::PowerConstrained)
                                      ? static_cast<WorkloadClass>(workloadValue)
                                      : WorkloadClass::Unknown;
            r.context.charging = std::stoi(charging) != 0;
            r.context.thermal_trend = trendFromInt(std::stoi(thermal));
            r.context.memory_trend = trendFromInt(std::stoi(memory));
            r.context.load_trend = trendFromInt(std::stoi(load));
            r.outcome = static_cast<BaselineIntelligence::Outcome>(std::stoi(outcome));
            r.score = clamp01(std::stod(score));
            r.confidence = clamp01(std::stod(confidence));
            r.observations = std::max<std::uint64_t>(1U, std::stoull(observations));
            loaded.push_back(std::move(r));
            if (loaded.size() >= kMaxRecords) break;
        }
        records_ = std::move(loaded);
        dirty_ = false;
        return true;
    } catch (...) {
        return false;
    }
}

bool ExperienceMemory::flush(const std::string& path) noexcept {
    if (!dirty_) return true;
    try {
        const std::string tmp = path + ".tmp";
        std::ofstream file(tmp, std::ios::out | std::ios::trunc);
        if (!file) return false;
        file << "COREFLOW_EXPERIENCE_V2\n";
        const std::string scope = safeToken(scope_) ? scope_ : "default";
        for (const Record& r : records_) {
            if (!safeToken(r.candidate.key)) continue;
            file << scope << '\t' << r.candidate.key << '\t'
                 << enumValue(r.context.state) << '\t'
                 << static_cast<int>(r.context.workload) << '\t'
                 << (r.context.charging ? 1 : 0) << '\t'
                 << enumValue(r.context.thermal_trend) << '\t'
                 << enumValue(r.context.memory_trend) << '\t'
                 << enumValue(r.context.load_trend) << '\t'
                 << static_cast<int>(r.outcome) << '\t'
                 << r.score << '\t' << r.confidence << '\t'
                 << r.observations << '\n';
        }
        file.flush();
        if (!file.good()) return false;
        file.close();
        if (std::rename(tmp.c_str(), path.c_str()) != 0) return false;
        dirty_ = false;
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace coreflow
