#include "coreflow/interactivity_boost.hpp"

#include <dirent.h>
#include <algorithm>
#include <string_view>
#include <vector>

namespace coreflow {
namespace {

[[nodiscard]] bool parseUint64(std::string_view raw, std::uint64_t& out) noexcept {
    while (!raw.empty() &&
           (raw.front() == ' ' || raw.front() == '\t' ||
            raw.front() == '\n' || raw.front() == '\r')) {
        raw.remove_prefix(1);
    }
    while (!raw.empty() &&
           (raw.back() == ' ' || raw.back() == '\t' ||
            raw.back() == '\n' || raw.back() == '\r')) {
        raw.remove_suffix(1);
    }
    if (raw.empty()) return false;
    std::uint64_t value = 0;
    for (const char c : raw) {
        if (c < '0' || c > '9') return false;
        value = value * 10U + static_cast<std::uint64_t>(c - '0');
    }
    out = value;
    return true;
}

// The frequency `steps` above `current` in a space-separated
// available_frequencies list. Fails when no higher step exists.
[[nodiscard]] bool bumpedFrequency(std::string_view available,
                                   std::uint64_t current, int steps,
                                   std::string& requested) noexcept {
    std::vector<std::uint64_t> frequencies;
    std::size_t i = 0;
    while (i < available.size()) {
        while (i < available.size() &&
               (available[i] == ' ' || available[i] == '\t' || available[i] == '\n')) {
            ++i;
        }
        std::uint64_t value = 0;
        bool any = false;
        while (i < available.size() && available[i] >= '0' && available[i] <= '9') {
            value = value * 10U + static_cast<std::uint64_t>(available[i] - '0');
            any = true;
            ++i;
        }
        if (any) frequencies.push_back(value);
        while (i < available.size() && available[i] != ' ' &&
               available[i] != '\t' && available[i] != '\n') {
            ++i;
        }
    }
    if (frequencies.empty()) return false;
    std::sort(frequencies.begin(), frequencies.end());
    std::uint64_t chosen = current;
    int taken = 0;
    for (const std::uint64_t frequency : frequencies) {
        if (frequency > chosen) {
            chosen = frequency;
            ++taken;
            if (taken >= steps) break;
        }
    }
    if (taken == 0) return false;
    requested = std::to_string(chosen);
    return true;
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

void InteractivityBoostController::discoverReadaheadPaths() noexcept {
    if (DIR* dir = ::opendir("/sys/block")) {
        while (dirent* entry = ::readdir(dir)) {
            const std::string name = entry->d_name;
            if (name == "." || name == "..") continue;
            const std::string path =
                "/sys/block/" + name + "/queue/read_ahead_kb";
            if (ResourceActuator::writable(path)) readahead_paths_.push_back(path);
        }
        ::closedir(dir);
    }
}

bool InteractivityBoostController::commitJournal() noexcept {
    if (journal_ == nullptr) return false;
    MutationJournal::Entries entries;
    entries.reserve(active_.size());
    for (const auto& item : active_) {
        entries.emplace_back(item.first, item.second.baseline);
    }
    if (entries.empty()) return journal_->clear();
    return journal_->commit(entries);
}

void InteractivityBoostController::expire(std::uint64_t now_ms) noexcept {
    bool changed = false;
    for (auto it = active_.begin(); it != active_.end();) {
        if (now_ms < it->second.deadline_ms) {
            ++it;
            continue;
        }

        // Keep the baseline and journal entry until restore is verified. A
        // transient sysfs failure must not discard the only recovery record.
        const ActuatorResult result = actuator_.restore(it->first, it->second.baseline);
        if (result.succeeded()) {
            it = active_.erase(it);
            changed = true;
        } else {
            ++it;
        }
    }
    if (changed) (void)commitJournal();
}

void InteractivityBoostController::restoreAll() noexcept {
    for (auto it = active_.begin(); it != active_.end();) {
        const ActuatorResult result = actuator_.restore(it->first, it->second.baseline);
        if (result.succeeded()) {
            it = active_.erase(it);
        } else {
            // Preserve failed restores in active_ and in the durable journal
            // so the next tick/recovery pass can retry them.
            ++it;
        }
    }
    (void)commitJournal(); // clears only when every restore succeeded
}

bool InteractivityBoostController::resolveTarget(const BoostIntent& intent,
                                                 std::string& path,
                                                 std::string& requested) noexcept {
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
}

std::size_t InteractivityBoostController::apply(
    const std::vector<BoostIntent>& intents, const EngineConfig& config,
    std::uint64_t now_ms) noexcept {
    expire(now_ms);
    if (!authorized(config) || journal_ == nullptr) return 0;

    std::size_t applied = 0;
    for (const auto& intent : intents) {
        if (intent.window_ms == 0U) continue;
        std::string path, requested;
        if (!resolveTarget(intent, path, requested)) continue;

        const auto existing = active_.find(path);
        if (existing != active_.end() &&
            existing->second.deadline_ms - now_ms > intent.window_ms / 2U) {
            continue; // anti-churn: a fresh boost is not renewed
        }
        std::string baseline;
        if (!ResourceActuator::readComparable(path, baseline)) continue;
        if (baseline == requested) continue;

        MutationJournal::Entries previous;
        for (const auto& item : active_) {
            previous.emplace_back(item.first, item.second.baseline);
        }

        const ActuatorId id{domainFor(intent.kind), next_instance_++};
        const ActuatorCapability capability = actuator_.capability(id, path);
        PolicyPlan plan;
        plan.action = PolicyAction::Candidate;
        plan.mutation_eligible = true;
        const MutationPermit permit = authority_.authorize(
            plan, config, RuntimeState::Normal, 1.0, capability, path);
        const ActuatorMutation mutation{id, path, requested};
        if (!permit.validFor(mutation)) continue;

        // The journal (including the new baseline) must be durable before the
        // write; a commit failure fails closed without touching the device.
        active_.emplace(path, ActiveBoost{id, baseline, requested,
                                          now_ms + intent.window_ms});
        if (!commitJournal()) {
            active_.erase(path);
            continue;
        }
        const ActuatorResult result = actuator_.apply(mutation, permit);
        if (!result.succeeded()) {
            const ActuatorResult rollback = actuator_.restore(path, baseline);
            if (rollback.succeeded()) {
                active_.erase(path);
                if (previous.empty()) {
                    (void)journal_->clear();
                } else {
                    (void)journal_->commit(previous);
                }
            } else {
                // Rollback failed: retain the baseline and durable journal
                // entry so a later expiry/recovery pass can retry it.
                (void)commitJournal();
            }
            continue;
        }
        ++applied;
    }
    return applied;
}

} // namespace coreflow
