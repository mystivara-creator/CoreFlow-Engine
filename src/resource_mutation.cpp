#include "coreflow/resource_mutation.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <sstream>

namespace coreflow {
namespace {

std::string trim(std::string value) {
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || value.back() == '\r'))
        value.pop_back();
    std::size_t first = 0;
    while (first < value.size() && (value[first] == ' ' || value[first] == '\t')) ++first;
    if (first != 0) value.erase(0, first);
    return value;
}

bool parseInteger(const std::string& value, long long& out) {
    try {
        std::size_t used = 0;
        const long long parsed = std::stoll(trim(value), &used, 10);
        const std::string cleaned = trim(value);
        if (used != cleaned.size()) return false;
        out = parsed;
        return true;
    } catch (...) {
        return false;
    }
}

std::string boundedInteger(long long value, long long low, long long high) {
    return std::to_string(std::clamp(value, low, high));
}

std::string scaledInteger(const std::string& baseline, double multiplier,
                          long long low, long long high) {
    long long value = 0;
    if (!parseInteger(baseline, value)) return {};
    const long long requested = static_cast<long long>(std::llround(
        static_cast<double>(value) * multiplier));
    return boundedInteger(requested, low, high);
}

bool isRotationalPath(const std::string& ioPath) {
    std::string value;
    const std::string path = ioPath + "/rotational";
    if (!ResourceActuator::read(path, value)) return false;
    return trim(value) == "1";
}

bool schedulerCandidate(const IoDevice& device, RuntimeState state,
                        const RuntimeSample& sample, std::string& out) {
    std::string schedulers;
    const std::string path = device.path + "/queue/scheduler";
    if (!ResourceActuator::read(path, schedulers)) return false;

    const bool rotational = isRotationalPath(device.path);
    const bool pressure = state == RuntimeState::Pressure ||
                          sample.mem_available_ratio < 0.12;
    (void)pressure;

    // Prefer only schedulers explicitly exposed by the kernel. For non-rotating
    // storage, "none" is a natural low-overhead candidate when available.
    // For rotational storage, prefer mq-deadline when exposed.
    std::istringstream stream(schedulers);
    std::string token;
    std::string preferred = rotational ? "mq-deadline" : "none";
    while (stream >> token) {
        if (!token.empty() && token.front() == '[' && token.back() == ']')
            token = token.substr(1, token.size() - 2);
        if (token == preferred) {
            out = token;
            return true;
        }
    }
    return false;
}

} // namespace

void ResourceMutationController::setJournal(MutationJournal* journal) noexcept {
    journal_ = journal;
    recovery_pending_ = true;
}

bool ResourceMutationController::recoverIfNeeded() noexcept {
    if (!recovery_pending_) return true;
    if (journal_ == nullptr) {
        recovery_pending_ = false;
        return true;
    }

    MutationJournal::Entries entries;
    switch (journal_->load(entries)) {
        case MutationJournal::LoadState::Absent:
            recovery_pending_ = false;
            return true;
        case MutationJournal::LoadState::Corrupt:
            return false;
        case MutationJournal::LoadState::Pending:
            break;
    }

    bool restored = true;
    for (const auto& entry : entries) {
        if (!std::filesystem::exists(entry.first)) continue;
        const ActuatorResult result = actuator_.restore(entry.first, entry.second);
        if (!(result.status == ActuatorStatus::RolledBack || result.status == ActuatorStatus::NoChange || result.status == ActuatorStatus::Ok)) restored = false;
    }
    if (!restored) {
        mutated_ = true;
        restore_failed_ = true;
        return false;
    }
    if (!journal_->clear()) return false;
    recovery_pending_ = false;
    journal_committed_ = false;
    mutated_ = false;
    restore_failed_ = false;
    return true;
}

bool ResourceMutationController::captureBaseline(const DeviceProfile& profile) noexcept {
    if (!recoverIfNeeded() || mutated_) return false;

    baseline_.clear();
    dirty_resources_.clear();
    last_applied_resources_.clear();
    auto capture = [this](ResourceDomain domain, const std::string& name,
                          const std::string& path, bool readable, bool writable) {
        if (path.empty() || !readable || !writable) return;
        std::string value;
        if (!ResourceActuator::read(path, value)) return;
        baseline_[path] = {domain, name, path, trim(value), true};
    };

    capture(ResourceDomain::Memory, "vm.swappiness", profile.vm_swappiness.path,
            profile.vm_swappiness.readable, profile.vm_swappiness.writable);

    for (const auto& device : profile.io_devices) {
        capture(ResourceDomain::Io, device.name + ":read_ahead_kb",
                device.path + "/queue/read_ahead_kb",
                device.read_ahead_readable, device.read_ahead_writable);
        capture(ResourceDomain::Io, device.name + ":nr_requests",
                device.path + "/queue/nr_requests",
                device.nr_requests_readable, device.nr_requests_writable);
        capture(ResourceDomain::Io, device.name + ":scheduler",
                device.path + "/queue/scheduler",
                device.scheduler_readable, device.scheduler_writable);
    }

    baseline_captured_ = !baseline_.empty();
    return baseline_captured_;
}


bool endsWith(
    const std::string& value,
    const char* suffix) noexcept {

    const std::size_t suffixLength =
        std::char_traits<char>::length(suffix);

    return value.size() >= suffixLength &&
           value.compare(
               value.size() - suffixLength,
               suffixLength,
               suffix) == 0;
}

InterventionLevel requiredIntervention(
    ResourceDomain domain,
    const std::string& name) noexcept {

    if (domain == ResourceDomain::Memory &&
        name == "vm.swappiness") {
        return InterventionLevel::Low;
    }

    if (domain == ResourceDomain::Io) {
        if (endsWith(name, ":read_ahead_kb")) {
            return InterventionLevel::Low;
        }

        if (endsWith(name, ":nr_requests")) {
            return InterventionLevel::Moderate;
        }

        if (endsWith(name, ":scheduler")) {
            return InterventionLevel::High;
        }
    }

    // Unknown resource => fail closed.
    return InterventionLevel::High;
}

bool interventionAllows(
    InterventionLevel granted,
    InterventionLevel required) noexcept {

    if (granted == InterventionLevel::ObserveOnly) {
        return false;
    }

    return static_cast<std::uint8_t>(granted) >=
           static_cast<std::uint8_t>(required);
}

bool ResourceMutationController::selectCandidate(
    RuntimeState state,
    const RuntimeSample& sample,
    const DeviceProfile& profile,
    const PolicyPlan& policy,
    Candidate& out) const noexcept {

    if (policy.intervention == InterventionLevel::ObserveOnly) {
        return false;
    }

    if (state == RuntimeState::Idle ||
        state == RuntimeState::Pressure ||
        state == RuntimeState::ThermalGuard) {
        return false;
    }

    auto accept = [&](ResourceDomain domain,
                      const std::string& name) noexcept {
        return interventionAllows(
                   policy.intervention,
                   requiredIntervention(domain, name)) &&
               policy.allows(domain, name);
    };

    // --------------------------------------------------------
    // VM swappiness
    // --------------------------------------------------------
    if (profile.vm_swappiness.readable &&
        profile.vm_swappiness.writable &&
        sample.mem_total_kb > 0 &&
        sample.mem_available_ratio < 0.25) {

        const auto it =
            baseline_.find(profile.vm_swappiness.path);

        if (it != baseline_.end() && it->second.valid) {
            long long base = 0;

            if (parseInteger(it->second.value, base)) {
                const long long requested = std::clamp(
                    base +
                        (sample.mem_available_ratio < 0.12
                             ? 10LL
                             : 5LL),
                    0LL,
                    100LL);

                if (requested != base &&
                    accept(
                        ResourceDomain::Memory,
                        "vm.swappiness")) {

                    out = {
                        ResourceDomain::Memory,
                        "vm.swappiness",
                        it->second.path,
                        std::to_string(requested),
                        it->second.value
                    };

                    return true;
                }
            }
        }
    }

    // --------------------------------------------------------
    // I/O
    // --------------------------------------------------------
    const bool sustained =
        state == RuntimeState::Elevated ||
        sample.cpu_utilization >= 0.70 ||
        sample.load1 >= 1.50;

    if (!sustained) {
        return false;
    }

    for (const auto& device : profile.io_devices) {

        // ----------------------------------------------------
        // read_ahead_kb
        // ----------------------------------------------------
        const std::string readAheadPath =
            device.path + "/queue/read_ahead_kb";

        const auto ra =
            baseline_.find(readAheadPath);

        if (ra != baseline_.end() &&
            ra->second.valid &&
            device.read_ahead_readable &&
            device.read_ahead_writable) {

            const std::string name =
                device.name + ":read_ahead_kb";

            const std::string requested =
                scaledInteger(
                    ra->second.value,
                    1.25,
                    128,
                    4096);

            if (!requested.empty() &&
                requested != ra->second.value &&
                accept(ResourceDomain::Io, name)) {

                out = {
                    ResourceDomain::Io,
                    name,
                    readAheadPath,
                    requested,
                    ra->second.value
                };

                return true;
            }
        }

        // ----------------------------------------------------
        // nr_requests
        // ----------------------------------------------------
        const std::string requestsPath =
            device.path + "/queue/nr_requests";

        const auto nr =
            baseline_.find(requestsPath);

        if (nr != baseline_.end() &&
            nr->second.valid &&
            device.nr_requests_readable &&
            device.nr_requests_writable) {

            const std::string name =
                device.name + ":nr_requests";

            const std::string requested =
                scaledInteger(
                    nr->second.value,
                    1.20,
                    32,
                    1024);

            if (!requested.empty() &&
                requested != nr->second.value &&
                accept(ResourceDomain::Io, name)) {

                out = {
                    ResourceDomain::Io,
                    name,
                    requestsPath,
                    requested,
                    nr->second.value
                };

                return true;
            }
        }

        // ----------------------------------------------------
        // scheduler
        // ----------------------------------------------------
        const std::string schedulerPath =
            device.path + "/queue/scheduler";

        const auto sched =
            baseline_.find(schedulerPath);

        if (sched != baseline_.end() &&
            sched->second.valid &&
            device.scheduler_readable &&
            device.scheduler_writable) {

            const std::string name =
                device.name + ":scheduler";

            std::string requested;

            if (schedulerCandidate(
                    device,
                    state,
                    sample,
                    requested) &&
                requested != sched->second.value &&
                accept(ResourceDomain::Io, name)) {

                out = {
                    ResourceDomain::Io,
                    name,
                    schedulerPath,
                    requested,
                    sched->second.value
                };

                return true;
            }
        }
    }

    return false;
}

MutationJournal::Entries ResourceMutationController::factorySnapshot() const {
    MutationJournal::Entries entries;
    entries.reserve(dirty_resources_.size());
    for (const auto& path : dirty_resources_) {
        const auto it = baseline_.find(path);
        if (it != baseline_.end() && it->second.valid) {
            entries.emplace_back(path, it->second.value);
        }
    }
    return entries;
}

MutationJournal::Entries ResourceMutationController::journalEntriesWith(const std::string& path) const {
    MutationJournal::Entries entries = factorySnapshot();
    if (dirty_resources_.find(path) == dirty_resources_.end()) {
        const auto it = baseline_.find(path);
        if (it != baseline_.end() && it->second.valid) {
            entries.emplace_back(path, it->second.value);
        }
    }
    return entries;
}

bool ResourceMutationController::applyCandidate(const Candidate& candidate, const MutationPermit& permit) noexcept {
    if (journal_ == nullptr) return false;
    const MutationJournal::Entries entries = journalEntriesWith(candidate.path);
    if (entries.empty() || !journal_->commit(entries)) return false;
    journal_committed_ = true;

    ActuatorMutation mutation;
    mutation.id = {ActuatorDomain::Io, 0};
    if (candidate.domain == ResourceDomain::Memory) mutation.id.domain = ActuatorDomain::Vm;
    mutation.target = candidate.path;
    mutation.requested = candidate.requested;

    if (!permit.validFor(MutationPermit::Scope::Resource)) return false;
    const ActuatorResult result = actuator_.apply(mutation, permit);
    if (result.writes_attempted != 0 && result.status != ActuatorStatus::RolledBack) {
        mutated_ = true;
        dirty_resources_.insert(candidate.path);
    }
    if (result.changed()) {
        last_applied_resources_.clear();
        last_applied_resources_.emplace_back(candidate.path, candidate.requested);
    }
    return result.succeeded() && (result.status == ActuatorStatus::NoChange || result.changed());
}

MutationResult ResourceMutationController::apply(
    const RuntimeState state, const RuntimeSample& sample,
    const DeviceProfile& profile, const EngineConfig& config,
    const PolicyPlan& policy, const MutationPermit& permit) noexcept {
    ++cycle_;
    if (!recoverIfNeeded()) return MutationResult::Failed;
    if (!baseline_captured_ && !captureBaseline(profile)) return MutationResult::Skipped;
    if (restore_failed_) return restoreAll() ? MutationResult::RolledBack : MutationResult::Failed;

    const bool mutation_gate_open =
        permit.validFor(MutationPermit::Scope::Resource) &&
        config.mutationMode() == MutationMode::Adaptive &&
        config.mutationArmed() &&
        policy.mutation_eligible &&
        policy.intervention != InterventionLevel::ObserveOnly &&
        sample.confidence >= config.minConfidence();

    if (!mutation_gate_open) {
        if (!mutated_) {
            return MutationResult::Skipped;
        }

        return restoreAll()
            ? MutationResult::RolledBack
            : MutationResult::Failed;
    }
    Candidate candidate;

    if (!selectCandidate(
            state,
            sample,
            profile,
            policy,
            candidate)) {
        return MutationResult::Skipped;
    }

    if (!applyCandidate(candidate, permit)) {
        return MutationResult::Failed;
    }

    return MutationResult::Verified;
}

bool ResourceMutationController::restoreAll() noexcept {
    if (!mutated_ && !restore_failed_) return true;

    bool ok = true;
    for (const auto& path : dirty_resources_) {
        const auto item = baseline_.find(path);
        if (item == baseline_.end() || !item->second.valid || !std::filesystem::exists(path)) continue;
        const ActuatorResult result = actuator_.restore(path, item->second.value);
        if (!(result.status == ActuatorStatus::RolledBack || result.status == ActuatorStatus::NoChange || result.status == ActuatorStatus::Ok)) ok = false;
    }
    if (!ok) {
        restore_failed_ = true;
        mutated_ = true;
        return false;
    }
    if (journal_committed_ && journal_ != nullptr && !journal_->clear()) {
        restore_failed_ = true;
        mutated_ = true;
        return false;
    }
    journal_committed_ = false;
    restore_failed_ = false;
    mutated_ = false;
    dirty_resources_.clear();
    last_applied_resources_.clear();
    return true;
}

void ResourceMutationController::rejectLastMutation() noexcept {
    last_applied_resources_.clear();
}

bool ResourceMutationController::syncPolicyModel(ResourceStateModel& model) const noexcept {
    bool any = false;
    for (const auto& item : baseline_) {
        if (!item.second.valid) continue;
        ResourceState* state = model.find(item.second.domain, item.second.name);
        if (state == nullptr) continue;
        if (!model.setBaseline(item.second.domain, item.second.name, item.second.value)) continue;
        (void)model.setDesired(item.second.domain, item.second.name, item.second.value);
        any = model.setMutationPermitted(item.second.domain, item.second.name, true) || any;
    }
    return any;
}

MutationResult ResourceMutationController::apply(
    const RuntimeState state, const RuntimeSample& sample,
    const DeviceProfile& profile, const EngineConfig& config) noexcept {
    PolicyPlan plan;
    plan.action = PolicyAction::Candidate;
    plan.mutation_eligible = true;
    plan.candidates.push_back({ResourceDomain::Memory, "vm.swappiness", {},
                               sample.confidence, 1.0, PolicyAction::Candidate});
    for (const auto& device : profile.io_devices) {
        plan.candidates.push_back({ResourceDomain::Io, device.name + ":read_ahead_kb", {},
                                   sample.confidence, 1.0, PolicyAction::Candidate});
        plan.candidates.push_back({ResourceDomain::Io, device.name + ":nr_requests", {},
                                   sample.confidence, 1.0, PolicyAction::Candidate});
        plan.candidates.push_back({ResourceDomain::Io, device.name + ":scheduler", {},
                                   sample.confidence, 1.0, PolicyAction::Candidate});
    }
    const MutationAuthority authority;
    const MutationPermit permit = authority.authorize(
        plan, config, state, sample.confidence, MutationPermit::Scope::Resource);
    return apply(state, sample, profile, config, plan, permit);
}

} // namespace coreflow
