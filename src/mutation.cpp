#include "coreflow/mutation.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <string>
#include <utility>

namespace coreflow {
namespace {

constexpr const char* kGovernorSuffix = "/scaling_governor";
constexpr double kAdaptiveHysteresis = 0.10;
constexpr double kThermalComfortC = 35.0;
constexpr double kThermalPressureC = 43.0;
constexpr double kLoadNormalization = 8.0;

const char* knownGovernor(const std::string& governor) noexcept {
    if (governor == "performance") return "performance";
    if (governor == "walt") return "walt";
    if (governor == "schedutil") return "schedutil";
    if (governor == "conservative") return "conservative";
    if (governor == "powersave") return "powersave";
    return nullptr;
}

std::string governorPathOf(const std::string& policy_path) {
    return policy_path + kGovernorSuffix;
}

// Authoritative live read. Returns false if the value cannot be read, so the
// caller fails closed instead of planning against stale data.
bool readLiveGovernor(const std::string& path, std::string& out) noexcept {
    try {
        std::ifstream file(path);
        if (!file) return false;
        std::string value;
        if (!std::getline(file, value) || value.empty()) return false;
        out = std::move(value);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace

void MutationController::setJournal(MutationJournal* journal) noexcept {
    journal_ = journal;
    recovery_pending_ = true;
}

void MutationController::setExperienceMemory(const ExperienceMemory* memory) noexcept {
    experience_memory_ = memory;
}

bool MutationController::captureBaseline(const DeviceProfile& profile) noexcept {
    profile_ = &profile;

    // A journal from a previous run means live governors may be mutated.
    // Recover first; if that fails, refuse to adopt any baseline.
    if (!ensureRecovered(profile)) return false;

    // Never accept a mutated state as the factory baseline. A refused capture
    // must leave the existing baseline intact so restore can still run.
    if (mutated_) return false;

    baseline_captured_ = false;
    actuator_ready_ = false;

    baseline_.clear();
    rejected_.clear();
    for (const CpuPolicy& policy : profile.cpu_policies) {
        if (!policy.path.empty() && policy.readable &&
            policy.governor_writable && !policy.governor.empty()) {
            baseline_[governorPathOf(policy.path)] = {policy.governor, true};
        }
    }

    // After crash recovery the profile reflects the mutated state. The true
    // factory values are the ones the journal recorded and just restored.
    for (const auto& factory : recovered_factory_) {
        const auto entry = baseline_.find(factory.first);
        if (entry != baseline_.end()) entry->second = {factory.second, true};
    }
    recovered_factory_.clear();

    cpufreq_actuator_.setProfile(&profile);
    actuator_ready_ = cpufreq_actuator_.discover() == ActuatorStatus::Ok;
    baseline_captured_ = actuator_ready_ && !baseline_.empty();
    return baseline_captured_;
}

bool MutationController::ensureRecovered(const DeviceProfile& profile) noexcept {
    if (!recovery_pending_) return true;

    if (journal_ == nullptr) {
        recovery_pending_ = false;
        return true;
    }

    MutationJournal::Entries entries;
    switch (journal_->load(entries)) {
        case MutationJournal::LoadState::Absent:
            // No commit happened, so no write could have landed.
            recovery_pending_ = false;
            return true;
        case MutationJournal::LoadState::Corrupt:
            // Untrusted journal: block mutation until an operator intervenes.
            return false;
        case MutationJournal::LoadState::Pending:
            break;
    }

    cpufreq_actuator_.setProfile(&profile);
    recovered_factory_.clear();

    bool all_restored = true;
    for (const auto& entry : entries) {
        bool present = false;
        for (const CpuPolicy& policy : profile.cpu_policies) {
            if (entry.first == governorPathOf(policy.path)) {
                present = true;
                break;
            }
        }
        // Policy no longer exists on this boot: nothing to restore.
        if (!present) continue;

        const ActuatorResult result =
            cpufreq_actuator_.restoreTo(entry.first, entry.second);
        if (!result.succeeded()) all_restored = false;
    }

    if (!all_restored) {
        mutated_ = true;
        restore_failed_ = true;
        return false;
    }

    if (!journal_->clear()) return false;

    recovered_factory_ = std::move(entries);
    recovery_pending_ = false;
    mutated_ = false;
    journal_committed_ = false;
    restore_failed_ = false;
    return true;
}

bool MutationController::isRejected(
    const std::string& policy_path,
    const std::string& governor
) const noexcept {
    const auto policy = rejected_.find(policy_path);
    if (policy == rejected_.end()) return false;
    const auto entry = policy->second.find(governor);
    if (entry == policy->second.end()) return false;
    return decision_cycle_ - entry->second < kRejectionCooldownCycles;
}

double MutationController::governorScore(
    const std::string& governor,
    RuntimeState state,
    const RuntimeSample& sample
) const noexcept {
    if (knownGovernor(governor) == nullptr) return -1.0;

    const double workload = sample.cpu_utilization_available
        ? std::clamp(sample.cpu_utilization, 0.0, 1.0)
        : std::clamp(sample.load1 / kLoadNormalization, 0.0, 1.0);

    const double thermal = sample.thermal_available
        ? std::clamp(
            (static_cast<double>(sample.thermal_millidegrees) / 1000.0 -
             kThermalComfortC) / (kThermalPressureC - kThermalComfortC),
            0.0, 1.0)
        : 0.5;

    double score = 0.0;
    if (governor == "performance") {
        score = (0.70 * workload) + (0.30 * (1.0 - thermal));
    } else if (governor == "walt") {
        score = (0.60 * workload) + (0.25 * (1.0 - thermal)) + 0.15;
    } else if (governor == "schedutil") {
        score = (0.50 * workload) + (0.30 * (1.0 - thermal)) + 0.20;
    } else if (governor == "conservative") {
        score = (0.35 * (1.0 - workload)) + (0.45 * thermal) + 0.20;
    } else if (governor == "powersave") {
        score = (0.15 * (1.0 - workload)) + (0.70 * thermal) + 0.15;
    }

    switch (state) {
        case RuntimeState::ThermalGuard:
            if (governor == "powersave") score += 0.25;
            else if (governor == "conservative") score += 0.20;
            else if (governor == "schedutil") score += 0.10;
            else if (governor == "walt") score -= 0.10;
            else if (governor == "performance") score -= 0.30;
            break;
        case RuntimeState::Elevated:
            if (governor == "walt") score += 0.08;
            else if (governor == "schedutil" || governor == "performance") score += 0.05;
            else if (governor == "powersave") score -= 0.10;
            break;
        case RuntimeState::Warming:
            if (governor == "conservative" || governor == "schedutil") score += 0.08;
            else if (governor == "walt") score += 0.03;
            else if (governor == "performance") score -= 0.10;
            break;
        case RuntimeState::Normal:
            break;
        case RuntimeState::Idle:
        case RuntimeState::Pressure:
            return -1.0;
    }

    return std::clamp(score, 0.0, 1.0);
}

bool MutationController::buildPlan(
    RuntimeState state,
    const RuntimeSample& sample,
    const DeviceProfile& profile,
    std::vector<MutationPlanEntry>& plan
) const noexcept {
    plan.clear();

    for (const CpuPolicy& policy : profile.cpu_policies) {
        if (policy.path.empty() || !policy.readable || !policy.governor_writable) {
            continue;
        }

        const std::string governorPath = governorPathOf(policy.path);
        const auto baseline = baseline_.find(governorPath);
        if (baseline == baseline_.end() || !baseline->second.valid) continue;

        // Read the live governor. The discovery snapshot goes stale after the
        // first mutation and must never drive hysteresis or "already set" checks.
        std::string current;
        if (!readLiveGovernor(governorPath, current)) continue;

        const double currentScore = governorScore(current, state, sample);
        std::string bestGovernor;
        double bestScore = -1.0;

        for (const std::string& governor : policy.available_governors) {
            if (governor.empty() || isRejected(policy.path, governor)) continue;

            double score = governorScore(governor, state, sample);
            if (score < 0.0) continue;

            // Experience is advisory: blend only for an exact compatible record.
            if (experience_memory_ != nullptr) {
                ExperienceMemory::CandidateIdentity identity;
                identity.key = policy.path + ":" + governor;

                ExperienceMemory::Context context;
                context.state = state;
                context.charging = sample.charging;
                context.thermal_trend = sample.thermal_trend;
                context.memory_trend = sample.memory_trend;
                context.load_trend = sample.load_trend;

                if (const ExperienceMemory::Record* record =
                        experience_memory_->find(identity, context);
                    record != nullptr && record->confidence > 0.0) {
                    const double memoryWeight =
                        std::clamp(record->confidence * 0.30, 0.0, 0.30);
                    score = (score * (1.0 - memoryWeight)) +
                            (record->score * memoryWeight);
                }
            }

            if (score > bestScore) {
                bestScore = score;
                bestGovernor = governor;
            }
        }

        if (bestGovernor.empty() || bestScore < 0.0) continue;
        if (bestGovernor == current) continue;

        // Hysteresis against the live governor, so a marginal score difference
        // cannot cause flapping.
        if (currentScore >= 0.0 && bestScore < currentScore + kAdaptiveHysteresis) {
            continue;
        }

        plan.push_back({governorPath, bestGovernor, baseline->second.value});
    }

    return !plan.empty();
}

MutationJournal::Entries MutationController::factorySnapshot() const {
    MutationJournal::Entries entries;
    entries.reserve(baseline_.size());
    for (const auto& item : baseline_) {
        if (item.second.valid) entries.emplace_back(item.first, item.second.value);
    }
    return entries;
}

MutationResult MutationController::applyPlan(
    const std::vector<MutationPlanEntry>& plan
) noexcept {
    if (plan.empty() || !actuator_ready_) return MutationResult::Skipped;

    // Resource bound: one verified CPUFreq write per decision cycle.
    const MutationPlanEntry& entry = plan.front();

    // Durability before mutation. If this fails, nothing is written.
    if (!journal_committed_ && journal_ != nullptr) {
        if (!journal_->commit(factorySnapshot())) return MutationResult::Failed;
    }
    journal_committed_ = true;

    // A write may land even if verification later fails, so mark dirty first.
    mutated_ = true;

    ActuatorMutation mutation;
    mutation.id = cpufreq_actuator_.id();
    mutation.target = entry.path;
    mutation.requested = entry.target;

    const ActuatorResult result = cpufreq_actuator_.apply(mutation);

    if (!result.succeeded()) {
        return result.status == ActuatorStatus::RolledBack
            ? MutationResult::RolledBack
            : MutationResult::Failed;
    }

    if (!result.changed()) return MutationResult::Skipped;

    last_applied_governors_.clear();
    const std::string suffix = kGovernorSuffix;
    if (entry.path.size() >= suffix.size() &&
        entry.path.compare(entry.path.size() - suffix.size(), suffix.size(), suffix) == 0) {
        last_applied_governors_.emplace_back(
            entry.path.substr(0, entry.path.size() - suffix.size()),
            entry.target);
    }

    return MutationResult::Verified;
}

MutationResult MutationController::restoreGovernors() noexcept {
    if (!actuator_ready_) {
        return mutated_ ? MutationResult::Failed : MutationResult::Skipped;
    }

    bool restoredAny = false;
    bool failed = false;

    for (const auto& entry : baseline_) {
        if (!entry.second.valid) continue;

        const ActuatorResult result = cpufreq_actuator_.restore(entry.first);
        if (result.status == ActuatorStatus::NoChange) continue;
        if (!result.succeeded()) {
            failed = true;
            continue;
        }
        if (result.changed()) restoredAny = true;
    }

    if (failed) {
        mutated_ = true;
        restore_failed_ = true;
        return MutationResult::Failed;
    }

    // Fully restored. Only now is it safe to drop the journal.
    if (journal_committed_ && journal_ != nullptr) {
        if (!journal_->clear()) {
            mutated_ = true;
            restore_failed_ = true;
            return MutationResult::Failed;
        }
    }

    journal_committed_ = false;
    mutated_ = false;
    restore_failed_ = false;
    last_applied_governors_.clear();

    return restoredAny ? MutationResult::RolledBack : MutationResult::Skipped;
}

void MutationController::rejectLastMutation() noexcept {
    for (const auto& entry : last_applied_governors_) {
        rejected_[entry.first][entry.second] = decision_cycle_;
    }
    last_applied_governors_.clear();
}

MutationResult MutationController::apply(
    RuntimeState state,
    const RuntimeSample& sample,
    const DeviceProfile& profile,
    const EngineConfig& config
) noexcept {
    ++decision_cycle_;

    if (!ensureRecovered(profile)) return MutationResult::Failed;

    if (!baseline_captured_) {
        if (!captureBaseline(profile)) {
            return mutated_ ? MutationResult::Failed : MutationResult::Skipped;
        }
    }

    // While a previous restore is incomplete, plan nothing new. Retry the restore.
    if (restore_failed_) return restoreGovernors();

    // Idle and memory pressure restore the factory baseline.
    if (state == RuntimeState::Idle || state == RuntimeState::Pressure) {
        return restoreGovernors();
    }

    // Confidence and permission gate mutation before any sysfs write.
    if (!config.allowCpuGovernor() || sample.confidence < config.minConfidence()) {
        return restoreGovernors();
    }

    std::vector<MutationPlanEntry> plan;
    if (!buildPlan(state, sample, profile, plan)) return MutationResult::Skipped;

    return applyPlan(plan);
}

bool MutationController::restoreAll() noexcept {
    if (profile_ != nullptr && !ensureRecovered(*profile_)) return false;
    if (!baseline_captured_) return !mutated_;
    return restoreGovernors() != MutationResult::Failed;
}

} // namespace coreflow
