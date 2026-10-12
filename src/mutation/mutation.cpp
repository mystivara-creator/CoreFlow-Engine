#include "coreflow/mutation.hpp"
#include "coreflow/outcome.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <string>
#include <utility>

namespace coreflow {
namespace {

constexpr char kGovernorSuffix[] = "/scaling_governor";
constexpr std::size_t kGovernorSuffixSize = sizeof(kGovernorSuffix) - 1U;
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

void MutationController::setRetainedStateStore(RetainedStateStore* store) noexcept {
    retained_store_ = store;
    retained_entries_.clear();
    retained_active_governors_.clear();
    active_governors_cache_.clear();
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
    retained_entries_.clear();
    retained_active_governors_.clear();
    active_governors_cache_.clear();

    baseline_.clear();
    dirty_governors_.clear();
    last_applied_governors_.clear();
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
    if (!actuator_ready_ || baseline_.empty()) return false;

    // Load accepted settings separately from the crash-recovery journal. The
    // retained ledger is never used as mutation authority: every entry must
    // match this device scope, a currently discovered governor path, and an
    // advertised governor value before it can influence restoration state.
    if (retained_store_ != nullptr) {
        RetainedStateStore::Entries entries;
        const RetainedStateStore::LoadState load_state = retained_store_->load(entries);
        if (load_state == RetainedStateStore::LoadState::Corrupt) return false;
        retained_entries_ = std::move(entries);
        const std::string scope = retainedScopeFor(profile);
        std::vector<std::string> stale_paths;
        for (const auto& retained : retained_entries_) {
            if (retained.scope != scope) {
                stale_paths.push_back(retained.path);
                continue;
            }
            const auto baseline_it = baseline_.find(retained.path);
            const CpuPolicy* owner = nullptr;
            for (const CpuPolicy& candidate : profile.cpu_policies) {
                if (governorPathOf(candidate.path) == retained.path) {
                    owner = &candidate;
                    break;
                }
            }
            if (baseline_it == baseline_.end() || owner == nullptr ||
                std::find(owner->available_governors.begin(), owner->available_governors.end(),
                          retained.baseline) == owner->available_governors.end() ||
                std::find(owner->available_governors.begin(), owner->available_governors.end(),
                          retained.target) == owner->available_governors.end()) {
                stale_paths.push_back(retained.path);
                continue;
            }
            std::string live;
            if (!readLiveGovernor(retained.path, live)) return false;
            if (live == retained.target) {
                baseline_it->second = {retained.baseline, true};
                dirty_governors_.insert(retained.path);
                mutated_ = true;
            } else if (live == retained.baseline) {
                baseline_it->second = {retained.baseline, true};
            } else {
                // An external actor changed this node to a third value. Do not
                // overwrite it based on a stale retained decision.
                baseline_it->second = {live, true};
                stale_paths.push_back(retained.path);
            }
        }
        for (const std::string& path : stale_paths) {
            if (!retained_store_->erasePath(path)) return false;
            retained_entries_.erase(std::remove_if(retained_entries_.begin(), retained_entries_.end(),
                [&path](const RetainedStateEntry& entry) { return entry.path == path; }),
                retained_entries_.end());
        }
    }

    baseline_captured_ = !baseline_.empty();
    if (baseline_captured_) refreshRetainedActiveCache();
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
    dirty_governors_.clear();
    retained_active_governors_.clear();
    active_governors_cache_.clear();
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
                context.workload = sample.cpu_utilization_available && sample.cpu_utilization >= 0.80
                    ? WorkloadClass::CpuBound
                    : (sample.io_activity_available &&
                       (sample.io_read_kb_per_sec + sample.io_write_kb_per_sec) >= 4096.0
                           ? WorkloadClass::IoBound : WorkloadClass::Interactive);
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

            // A previously measured-beneficial setting gets only a small
            // preference. Current live scoring, advertised capability, policy,
            // permit and hysteresis remain authoritative.
            for (const auto& retained : retained_entries_) {
                if (retained.path == governorPath && retained.target == governor) {
                    score += 0.08;
                    break;
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
    entries.reserve(dirty_governors_.size());
    for (const auto& path : dirty_governors_) {
        const auto it = baseline_.find(path);
        if (it != baseline_.end() && it->second.valid) {
            entries.emplace_back(path, it->second.value);
        }
    }
    return entries;
}

MutationJournal::Entries MutationController::journalEntriesWith(const std::string& path) const {
    MutationJournal::Entries entries = factorySnapshot();
    if (dirty_governors_.find(path) == dirty_governors_.end()) {
        const auto it = baseline_.find(path);
        if (it != baseline_.end() && it->second.valid) {
            entries.emplace_back(path, it->second.value);
        }
    }
    return entries;
}

MutationResult MutationController::applyPlan(
    const std::vector<MutationPlanEntry>& plan,
    const MutationPermit& permit
) noexcept {
    if (plan.empty() || !actuator_ready_) return MutationResult::Skipped;

    // A durable journal is mandatory for autonomous mutation. Without it, a
    // daemon crash could leave a changed governor with no recovery record.
    if (journal_ == nullptr) return MutationResult::Failed;

    if (!cpufreq_actuator_.capability().mutationSafe()) {
        return MutationResult::Failed;
    }

    // One verified CPUFreq write per decision cycle.
    const MutationPlanEntry& entry = plan.front();

    // Form the exact mutation. The permit MUST already be capability-bound
    // for this actuator + target (callers re-authorize after buildPlan).
    ActuatorMutation mutation;
    mutation.id = cpufreq_actuator_.id();
    mutation.target = entry.path;
    mutation.requested = entry.target;

    if (!permit.validFor(mutation)) {
        return MutationResult::Skipped;
    }

    std::string previous_value;
    if (!readLiveGovernor(entry.path, previous_value)) return MutationResult::Failed;
    const bool was_dirty_before = dirty_governors_.find(entry.path) != dirty_governors_.end();
    const bool from_retained = std::any_of(retained_entries_.begin(), retained_entries_.end(),
        [&entry](const RetainedStateEntry& retained) {
            return retained.path == entry.path && retained.target == entry.target;
        });

    // Journal only after a valid capability-bound permit is confirmed.
    // The journal holds factory values for every path this controller owns
    // (plus the candidate about to change).
    const bool journal_was_committed = journal_committed_;
    const MutationJournal::Entries journal_entries = journalEntriesWith(entry.path);
    if (journal_entries.empty() || journal_ == nullptr || !journal_->commit(journal_entries)) {
        return MutationResult::Failed;
    }
    journal_committed_ = true;

    const ActuatorResult result = cpufreq_actuator_.apply(mutation, permit);

    // A write may have landed even when verification failed. Track the
    // touched policy individually; only an explicit RolledBack clears it.
    if (result.writes_attempted != 0 && result.status != ActuatorStatus::RolledBack) {
        dirty_governors_.insert(entry.path);
        mutated_ = true;
    }

    if (result.status == ActuatorStatus::RolledBack) {
        // The actuator returns to the pre-write value. Keep earlier retained
        // ownership for this path if it was already dirty before this attempt.
        if (!was_dirty_before) dirty_governors_.erase(entry.path);
        mutated_ = !dirty_governors_.empty();
        last_applied_governors_.clear();
        if (journal_committed_ && journal_ != nullptr) {
            const MutationJournal::Entries snapshot = factorySnapshot();
            const bool journal_ok = snapshot.empty() ? journal_->clear()
                                                     : journal_->commit(snapshot);
            if (!journal_ok) {
                restore_failed_ = true;
                mutated_ = true;
                return MutationResult::Failed;
            }
            journal_committed_ = !snapshot.empty();
        }
        return MutationResult::RolledBack;
    }

    if (!result.succeeded()) {
        return MutationResult::Failed;
    }

    if (!result.changed()) {
        // No kernel mutation occurred. If this decision opened a fresh journal
        // epoch, clear it; do not turn a NoChange result into a false mutation.
        if (!journal_was_committed && journal_committed_ && journal_ != nullptr) {
            if (journal_->clear()) {
                journal_committed_ = false;
            } else {
                // Keep the durable journal as a conservative recovery record.
                // No restore is attempted because CoreFlow did not mutate.
                return MutationResult::Failed;
            }
        }
        return MutationResult::Skipped;
    }

    last_applied_governors_.clear();
    pending_outcome_ = true;
    last_mutation_path_ = entry.path;
    last_mutation_target_ = entry.target;
    last_previous_value_ = previous_value;
    last_path_was_dirty_before_ = was_dirty_before;
    last_candidate_from_retained_ = from_retained;
    const std::string suffix = kGovernorSuffix;
    if (entry.path.size() >= suffix.size() &&
        entry.path.compare(entry.path.size() - suffix.size(), suffix.size(), suffix) == 0) {
        last_applied_governors_.emplace_back(
            entry.path.substr(0, entry.path.size() - suffix.size()),
            entry.target);
    }
    refreshActiveSettingsCache();

    return MutationResult::Verified;
}

MutationResult MutationController::restoreGovernors() noexcept {
    if (!actuator_ready_) {
        return mutated_ ? MutationResult::Failed : MutationResult::Skipped;
    }

    bool restoredAny = false;
    bool failed = false;

    for (const auto& path : dirty_governors_) {
        const auto entry = baseline_.find(path);
        if (entry == baseline_.end() || !entry->second.valid) continue;

        const ActuatorResult result = cpufreq_actuator_.restoreTo(path, entry->second.value);
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
    dirty_governors_.clear();
    retained_active_governors_.clear();
    active_governors_cache_.clear();
    mutated_ = false;
    restore_failed_ = false;
    last_applied_governors_.clear();
    pending_outcome_ = false;
    last_mutation_path_.clear();
    last_mutation_target_.clear();
    last_previous_value_.clear();
    last_path_was_dirty_before_ = false;
    last_candidate_from_retained_ = false;

    return restoredAny ? MutationResult::RolledBack : MutationResult::Skipped;
}

bool MutationController::retainLastMutation(const DeviceProfile& profile) noexcept {
    if (!pending_outcome_ || retained_store_ == nullptr ||
        last_mutation_path_.empty() || last_mutation_target_.empty()) return false;
    const auto baseline = baseline_.find(last_mutation_path_);
    if (baseline == baseline_.end() || !baseline->second.valid ||
        baseline->second.value == last_mutation_target_) return false;
    std::string live;
    if (!readLiveGovernor(last_mutation_path_, live) || live != last_mutation_target_) return false;

    RetainedStateEntry entry;
    entry.scope = retainedScopeFor(profile);
    entry.path = last_mutation_path_;
    entry.baseline = baseline->second.value;
    entry.target = last_mutation_target_;
    RetainedStateStore::Entries updated_entries;
    try {
        updated_entries = retained_entries_;
        updated_entries.erase(std::remove_if(updated_entries.begin(), updated_entries.end(),
            [&entry](const RetainedStateEntry& current) { return current.path == entry.path; }),
            updated_entries.end());
        updated_entries.push_back(entry);
    } catch (...) {
        return false;
    }
    if (!retained_store_->upsert(entry)) {
        // A rename may have succeeded even if the final directory sync failed.
        // Best-effort restore the last known-good ledger so an unaccepted
        // candidate cannot become a retained preference after restart.
        (void)retained_store_->commit(retained_entries_);
        return false;
    }
    retained_entries_.swap(updated_entries);
    pending_outcome_ = false;
    last_applied_governors_.clear();
    refreshRetainedActiveCache();
    last_mutation_path_.clear();
    last_mutation_target_.clear();
    last_previous_value_.clear();
    last_path_was_dirty_before_ = false;
    last_candidate_from_retained_ = false;
    return true;
}

bool MutationController::rollbackLastMutation() noexcept {
    if (!pending_outcome_ || last_mutation_path_.empty() || last_previous_value_.empty()) {
        return false;
    }

    std::string live;
    if (!readLiveGovernor(last_mutation_path_, live)) {
        mutated_ = true;
        restore_failed_ = true;
        return false;
    }

    bool external_override = false;
    bool storage_ok = true;
    if (live == last_mutation_target_) {
        const ActuatorResult result = cpufreq_actuator_.restoreTo(
            last_mutation_path_, last_previous_value_);
        if (!(result.succeeded() || result.status == ActuatorStatus::RolledBack)) {
            mutated_ = true;
            restore_failed_ = true;
            return false;
        }
    } else if (live != last_previous_value_) {
        // Another actor changed the node after CoreFlow's write. Preserve that
        // live value rather than overwriting it with a stale rollback target.
        external_override = true;
        const auto baseline = baseline_.find(last_mutation_path_);
        if (baseline != baseline_.end()) baseline->second = {live, true};
        dirty_governors_.erase(last_mutation_path_);
        if (retained_store_ != nullptr) {
            storage_ok = retained_store_->erasePath(last_mutation_path_);
        }
        retained_entries_.erase(std::remove_if(retained_entries_.begin(), retained_entries_.end(),
            [this](const RetainedStateEntry& entry) {
                return entry.path == last_mutation_path_;
            }), retained_entries_.end());
    }

    if (!external_override && !last_path_was_dirty_before_) {
        dirty_governors_.erase(last_mutation_path_);
    }
    mutated_ = !dirty_governors_.empty();

    if (journal_ != nullptr && journal_committed_) {
        const MutationJournal::Entries snapshot = factorySnapshot();
        const bool journal_ok = snapshot.empty() ? journal_->clear()
                                                 : journal_->commit(snapshot);
        if (!journal_ok) {
            restore_failed_ = true;
            mutated_ = true;
            return false;
        }
        journal_committed_ = !snapshot.empty();
    }

    if (last_candidate_from_retained_ && !last_path_was_dirty_before_ &&
        !external_override && retained_store_ != nullptr) {
        if (!retained_store_->erasePath(last_mutation_path_)) storage_ok = false;
        retained_entries_.erase(std::remove_if(retained_entries_.begin(), retained_entries_.end(),
            [this](const RetainedStateEntry& entry) {
                return entry.path == last_mutation_path_;
            }), retained_entries_.end());
    }

    pending_outcome_ = false;
    last_applied_governors_.clear();
    refreshRetainedActiveCache();
    last_mutation_path_.clear();
    last_mutation_target_.clear();
    last_previous_value_.clear();
    last_path_was_dirty_before_ = false;
    last_candidate_from_retained_ = false;
    if (!storage_ok) {
        restore_failed_ = true;
        mutated_ = true;
        return false;
    }
    return true;
}

void MutationController::refreshRetainedActiveCache() noexcept {
    try {
        retained_active_governors_.clear();
        for (const auto& retained : retained_entries_) {
            std::string live;
            if (!readLiveGovernor(retained.path, live) || live != retained.target) continue;
            if (retained.path.size() < kGovernorSuffixSize ||
                retained.path.compare(retained.path.size() - kGovernorSuffixSize,
                                      kGovernorSuffixSize, kGovernorSuffix) != 0) continue;
            retained_active_governors_.emplace_back(
                retained.path.substr(0, retained.path.size() - kGovernorSuffixSize),
                retained.target);
        }
        refreshActiveSettingsCache();
    } catch (...) {
        retained_active_governors_.clear();
        active_governors_cache_.clear();
    }
}

void MutationController::refreshActiveSettingsCache() noexcept {
    try {
        active_governors_cache_ = retained_active_governors_;
        if (!pending_outcome_) return;
        for (const auto& candidate : last_applied_governors_) {
            const auto same_path = std::find_if(active_governors_cache_.begin(),
                active_governors_cache_.end(), [&candidate](const auto& current) {
                    return current.first == candidate.first;
                });
            if (same_path == active_governors_cache_.end()) {
                active_governors_cache_.push_back(candidate);
            } else {
                *same_path = candidate;
            }
        }
    } catch (...) {
        active_governors_cache_.clear();
    }
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
    const EngineConfig& config,
    const PolicyPlan& policy,
    const MutationPermit& permit
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
        return relaxToBaseline();
    }

    // Coarse safety gate (mode / arm / policy / confidence). The concrete
    // capability-bound permit is issued after the candidate path is known.
    if (config.mutationMode() != MutationMode::Adaptive ||
        !config.mutationArmed() || !config.allowCpuGovernor() ||
        !policy.mutation_eligible || sample.confidence < config.minConfidence()) {
        return relaxToBaseline();
    }
    // Accept either a capability-bound permit already issued for a CPU target,
    // or a legacy scope permit that will be upgraded below.
    if (!(permit.isCapabilityBound() ? permit.valid()
                                     : permit.validFor(MutationPermit::Scope::CpuFreq))) {
        return relaxToBaseline();
    }

    // One candidate must finish its outcome window before another is applied.
    // Previously accepted settings may remain active while a new candidate is
    // evaluated; their existence alone does not block further adaptation.
    if (pending_outcome_) return MutationResult::Skipped;

    std::vector<MutationPlanEntry> plan_entries;
    if (!buildPlan(state, sample, profile, plan_entries)) return MutationResult::Skipped;
    if (plan_entries.empty()) return MutationResult::Skipped;

    // Issue (or re-issue) a capability-bound permit for the exact candidate.
    const ActuatorCapability cap = cpufreq_actuator_.capability();
    const MutationAuthority authority;
    const MutationPermit bound = authority.authorize(
        policy, config, state, sample.confidence, cap, plan_entries.front().path);
    if (!bound.validFor(ActuatorMutation{cap.id, plan_entries.front().path,
                                         plan_entries.front().target})) {
        return MutationResult::Skipped;
    }

    return applyPlan(plan_entries, bound);
}

MutationResult MutationController::relaxToBaseline() noexcept {
    if (!mutated_ && !restore_failed_) return MutationResult::Skipped;
    return restoreGovernors();
}

bool MutationController::restoreAll() noexcept {
    if (profile_ != nullptr && !ensureRecovered(*profile_)) return false;
    if (!baseline_captured_) return !mutated_;
    // Never write to the kernel to "correct" a governor this controller did not change.
    if (!mutated_ && !restore_failed_) return true;
    return restoreGovernors() != MutationResult::Failed;
}

MutationResult MutationController::apply(
    RuntimeState state,
    const RuntimeSample& sample,
    const DeviceProfile& profile,
    const EngineConfig& config
) noexcept {
    PolicyPlan plan;
    plan.action = PolicyAction::Candidate;
    plan.mutation_eligible = true;
    // Convenience path for deterministic tests: obtain a coarse scope permit.
    // The full apply() path re-authorizes a capability-bound permit once the
    // concrete candidate path is known by buildPlan.
    const MutationAuthority authority;
    const MutationPermit permit = authority.authorize(
        plan, config, state, sample.confidence, MutationPermit::Scope::CpuFreq);
    return apply(state, sample, profile, config, plan, permit);
}

} // namespace coreflow

// Outcome classification is colocated with mutation orchestration.
namespace coreflow {
OutcomeClass classifyOutcome(const ActuatorResult& result) noexcept {
    switch (result.status) {
        case ActuatorStatus::Ok:
            return result.changed() ? OutcomeClass::Neutral : OutcomeClass::Neutral;
        case ActuatorStatus::NoChange:
            return OutcomeClass::Neutral;
        case ActuatorStatus::SafetyRejected:
        case ActuatorStatus::ValidationFailed:
            return OutcomeClass::SafetyBlocked;
        case ActuatorStatus::RolledBack:
            return OutcomeClass::RolledBack;
        case ActuatorStatus::WriteFailed:
        case ActuatorStatus::VerifyFailed:
        case ActuatorStatus::ReadFailed:
            return OutcomeClass::Failed;
        case ActuatorStatus::Unsupported:
        case ActuatorStatus::Unavailable:
        case ActuatorStatus::Invalid:
            return OutcomeClass::Unknown;
    }
    return OutcomeClass::Unknown;
}

const char* outcomeClassName(OutcomeClass result) noexcept {
    switch (result) {
        case OutcomeClass::Unknown: return "UNKNOWN";
        case OutcomeClass::Neutral: return "NEUTRAL";
        case OutcomeClass::Beneficial: return "BENEFICIAL";
        case OutcomeClass::Regressed: return "REGRESSED";
        case OutcomeClass::SafetyBlocked: return "SAFETY_BLOCKED";
        case OutcomeClass::Failed: return "FAILED";
        case OutcomeClass::RolledBack: return "ROLLED_BACK";
    }
    return "UNKNOWN";
}

} // namespace coreflow
