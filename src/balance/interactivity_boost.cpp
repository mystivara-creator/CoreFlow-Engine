#include "coreflow/interactivity_boost.hpp"
#include "coreflow/integer_parse.hpp"

#include <algorithm>
#include <iterator>
#include <limits>
#include <string_view>
#include <vector>

namespace coreflow {

using integer_parse::bumpedFrequency;
using integer_parse::parseUint64;

namespace {

[[nodiscard]] std::uint64_t deadlineFrom(std::uint64_t now_ms,
                                         std::uint64_t window_ms) noexcept {
    if (window_ms > std::numeric_limits<std::uint64_t>::max() - now_ms) {
        return std::numeric_limits<std::uint64_t>::max();  // saturate, never wrap
    }
    return now_ms + window_ms;
}

// A restore that brought the baseline back. ResourceActuator::restore reports
// RolledBack on success, and mutation.cpp accepts that status for restores, so
// this controller must accept it too. Using succeeded() alone would treat every
// successful restore as a failure, and the boost would never leave the journal.
[[nodiscard]] bool restoreSucceeded(const ActuatorResult& result) noexcept {
    return result.succeeded() || result.status == ActuatorStatus::RolledBack;
}

// Each boost writes a domain that has its own operator grant. A capability that
// is present but not granted is never written.
[[nodiscard]] bool grantedFor(BoostKind kind, const EngineConfig& config) noexcept {
    switch (kind) {
        case BoostKind::GpuMinFreqStep: return config.grantGpu();
        case BoostKind::TopAppUclampMin: return config.grantCgroup();  // uclamp lives in the cpu controller
        case BoostKind::IoReadaheadKb: return config.grantBlockQueue();
    }
    return false;
}

[[nodiscard]] ActuatorDomain domainFor(BoostKind kind) noexcept {
    switch (kind) {
        case BoostKind::GpuMinFreqStep: return ActuatorDomain::Io; // devfreq node
        case BoostKind::TopAppUclampMin: return ActuatorDomain::Uclamp;
        case BoostKind::IoReadaheadKb: return ActuatorDomain::Io;
    }
    return ActuatorDomain::Io;
}

} // namespace

bool InteractivityBoostController::authorized(
    const EngineConfig& config) const noexcept {
    return config.interactivityMode() == InteractivityMode::Adaptive &&
           config.mutationMode() == MutationMode::Adaptive &&
           config.mutationArmed();
}

bool InteractivityBoostController::commitJournal() noexcept {
    try {
    if (journal_ == nullptr || journal_untrusted_) {
        recovery_required_ = true;
        return false;  // a corrupt journal is never overwritten
    }
    MutationJournal::Entries entries;
    entries.reserve(active_.size());
    for (const auto& item : active_) {
        if (!item.second.restored) entries.emplace_back(item.first, item.second.baseline);
    }
    const bool committed = entries.empty() ? journal_->clear() : journal_->commit(entries);
    recovery_required_ = !committed;
    return committed;
    } catch (...) {
        recovery_required_ = true;
        return false;
    }
}

std::size_t InteractivityBoostController::recoverFromJournal() noexcept {
    try {
    if (journal_ == nullptr) {
        recovery_required_ = true;
        return 0;
    }
    MutationJournal::Entries pending;
    switch (journal_->load(pending)) {
        case MutationJournal::LoadState::Absent:
            return 0;
        case MutationJournal::LoadState::Corrupt:
            journal_untrusted_ = true;
            recovery_required_ = true;
            return 0;
        case MutationJournal::LoadState::Pending:
            break;
    }
    std::size_t adopted = 0;
    for (const auto& entry : pending) {
        if (active_.find(entry.first) != active_.end()) continue;
        ActiveBoost boost;
        boost.baseline = entry.second;
        boost.deadline_ms = 0U;  // already due: the next expire() restores it
        active_.emplace(entry.first, std::move(boost));
        ++adopted;
    }
    return adopted;
    } catch (...) {
        recovery_required_ = true;
        return 0;
    }
}

bool InteractivityBoostController::settle() noexcept {
    if (!commitJournal()) return false;
    for (auto it = active_.begin(); it != active_.end();) {
        it = it->second.restored ? active_.erase(it) : std::next(it);
    }
    return true;
}

void InteractivityBoostController::expire(std::uint64_t now_ms) noexcept {
    bool restored_now = false;
    for (auto& item : active_) {
        ActiveBoost& boost = item.second;
        if (boost.restored || now_ms < boost.deadline_ms) continue;
        if (external_control_paths_ != nullptr && external_control_paths_->contains(item.first)) continue;
        // A failed restore keeps the entry. It stays journaled and blocks its
        // path until a later pass restores it.
        if (restoreSucceeded(actuator_.restore(item.first, boost.baseline))) {
            boost.restored = true;
            restored_now = true;
        }
    }
    if (restored_now || recovery_required_) (void)settle();
}

void InteractivityBoostController::restoreAll() noexcept {
    bool handoff_pending = false;
    for (auto& item : active_) {
        if (item.second.restored) continue;
        if (external_control_paths_ != nullptr && external_control_paths_->contains(item.first)) {
            handoff_pending = true; continue;
        }
        if (restoreSucceeded(actuator_.restore(item.first, item.second.baseline))) {
            item.second.restored = true;
        }
    }
    (void)settle();  // entries whose restore failed remain journaled
    if (handoff_pending) recovery_required_ = true;
}

bool InteractivityBoostController::reconcileExternalChanges(std::unordered_set<std::string>& paths) noexcept {
    try {
        for (auto it = active_.begin(); it != active_.end();) {
            if (it->second.restored) { ++it; continue; }
            std::string live;
            if (it->second.requested.empty() || !ResourceActuator::readComparable(it->first, live)) return false;
            if (live == ResourceActuator::comparableValue(it->second.requested) && !paths.contains(it->first)) {
                ++it; continue;
            }
            paths.insert(it->first);
            if (journal_ == nullptr || journal_untrusted_ ||
                !journal_->releaseExternallyControlledPath(it->first)) {
                recovery_required_ = true;
                return false;
            }
            it = active_.erase(it); // journal handoff completed; never restore the external value
        }
        return settle();
    } catch (...) { recovery_required_ = true; return false; }
}

ControlContinuity InteractivityBoostController::inspectContinuity() const noexcept {
    try {
        bool unavailable = false;
        for (const auto& [path, boost] : active_) {
            if (boost.restored) continue;
            if (boost.requested.empty()) { unavailable = true; continue; }
            std::string live;
            if (!ResourceActuator::readComparable(path, live)) { unavailable = true; continue; }
            if (live != boost.requested) return ControlContinuity::Changed;
        }
        return unavailable ? ControlContinuity::Unavailable : ControlContinuity::Intact;
    } catch (...) { return ControlContinuity::Unavailable; }
}

bool InteractivityBoostController::resolveTarget(const BoostIntent& intent,
                                                 std::string& path,
                                                 std::string& requested) noexcept {
    try {
    switch (intent.kind) {
        case BoostKind::GpuMinFreqStep: {
            if (gpu_min_freq_path_.empty()) return false;
            std::string current, available;
            if (!ResourceActuator::readComparable(gpu_min_freq_path_, current)) {
                return false;
            }
            const std::string dir =
                gpu_min_freq_path_.substr(0, gpu_min_freq_path_.find_last_of('/'));
            if (!ResourceActuator::readComparable(dir + "/available_frequencies",
                                                  available) &&
                !ResourceActuator::readComparable(dir + "/gpu_freq_table",
                                                  available) &&
                !ResourceActuator::readComparable(dir + "/gpu_available_frequencies",
                                                  available)) {
                return false;
            }
            std::uint64_t current_hz = 0;
            if (!parseUint64(current, current_hz)) return false;
            const int steps = intent.value > 0L ? static_cast<int>(intent.value) : 1;
            if (!bumpedFrequency(available, current_hz, steps, requested)) {
                return false;
            }
            path = gpu_min_freq_path_;
            return true;
        }
        case BoostKind::TopAppUclampMin: {
            if (uclamp_path_.empty()) return false;
            requested = std::to_string(std::clamp(intent.value, 0L, 100L));
            path = uclamp_path_;
            return true;
        }
        case BoostKind::IoReadaheadKb: {
            for (const auto& candidate : readahead_paths_) {
                if (external_control_paths_ != nullptr && external_control_paths_->contains(candidate)) continue;
                if (active_.find(candidate) != active_.end()) continue;
                std::string current;
                if (!ResourceActuator::readComparable(candidate, current)) continue;
                std::uint64_t current_kb = 0;
                if (!parseUint64(current, current_kb)) continue;
                if (static_cast<std::uint64_t>(std::max(intent.value, 0L)) <=
                    current_kb) {
                    continue;
                }
                requested = std::to_string(intent.value);
                path = candidate;
                return true;
            }
            return false;
        }
    }
    return false;

    } catch (...) {
        path.clear();
        requested.clear();
        return false;
    }
}
std::size_t InteractivityBoostController::apply(
    const std::vector<BoostIntent>& intents, const EngineConfig& config,
    std::uint64_t now_ms) noexcept {
    expire(now_ms);
    if (!authorized(config) || journal_ == nullptr || recovery_required_) return 0;

    std::size_t applied = 0;
    for (const auto& intent : intents) {
        if (intent.window_ms == 0U) continue;
        if (!grantedFor(intent.kind, config)) continue;
        std::string path, requested;
        if (!resolveTarget(intent, path, requested)) continue;
        if (external_control_paths_ != nullptr && external_control_paths_->contains(path)) continue;

        // One boost per path. A path that is still boosted, or whose restore is
        // pending, is never touched: its current value is the boosted one, and
        // taking it as a new baseline would lose the original.
        if (active_.find(path) != active_.end()) continue;

        std::string baseline;
        if (!ResourceActuator::readComparable(path, baseline)) continue;
        if (baseline == requested) continue;

        const ActuatorId id{domainFor(intent.kind), next_instance_++};
        const ActuatorCapability capability = actuator_.capability(id, path);
        PolicyPlan plan;
        plan.action = PolicyAction::Candidate;
        plan.mutation_eligible = true;
        const MutationPermit permit = authority_.authorize(
            plan, config, RuntimeState::Normal, 1.0, capability, path);
        const ActuatorMutation mutation{id, path, requested};
        if (!permit.validFor(mutation)) continue;

        // The journal, with the new baseline, must be durable before the write.
        // If it cannot be committed, no write happens and the batch stops.
        active_.emplace(path, ActiveBoost{id, baseline, requested,
                                          deadlineFrom(now_ms, intent.window_ms)});
        if (!commitJournal()) {
            active_.erase(path);
            return applied;
        }

        const ActuatorResult result = actuator_.apply(mutation, permit);
        if (!result.succeeded()) {
            // Roll back. If the rollback fails, the entry stays pending and
            // journaled, and its path is blocked until recovery restores it.
            if (restoreSucceeded(actuator_.restore(path, baseline))) {
                active_.erase(path);
            }
            (void)settle();
            continue;
        }
        ++applied;
    }
    return applied;
}

} // namespace coreflow
