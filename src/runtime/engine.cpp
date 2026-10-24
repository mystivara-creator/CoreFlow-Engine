#include "coreflow/runtime/engine.hpp"
#include "coreflow/runtime/limits.hpp"
#include "coreflow/internal/rollback_guard.hpp"
#include "coreflow/optimization/workload.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <thread>
#include <unordered_map>

namespace coreflow {
namespace {
using namespace runtime_limits;

std::int64_t unixNow() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

bool parseFinite(const std::string& text, double& value) {
    char* end = nullptr;
    value = std::strtod(text.c_str(), &end);
    return end != text.c_str() && end && *end == '\0' && std::isfinite(value);
}

void refreshObservedValues(std::vector<Capability>& capabilities, const ObservationFrame& frame) {
    std::unordered_map<std::string, std::string> values;
    values.reserve(frame.capability_values.size());
    for (const auto& value : frame.capability_values) values.emplace(value.first, value.second);
    for (auto& capability : capabilities) {
        const auto found = values.find(capability.id);
        if (found == values.end()) {
            capability.optimization_supported = false;
            capability.numeric = false;
            continue;
        }
        if (!capability.choices.empty()) {
            if (std::find(capability.choices.begin(), capability.choices.end(), found->second) !=
                capability.choices.end()) {
                capability.raw_value = found->second;
            } else {
                capability.optimization_supported = false;
            }
            continue;
        }
        double numeric{};
        if (!parseFinite(found->second, numeric)) {
            capability.numeric = false;
            capability.optimization_supported = false;
            continue;
        }
        capability.raw_value = found->second;
        capability.numeric_value = numeric;
        capability.numeric = true;
    }
}

std::vector<CapabilityRecord> toRecords(const std::vector<Capability>& capabilities) {
    std::vector<CapabilityRecord> records;
    records.reserve(std::min(capabilities.size(), kWebUiLimit));
    for (const auto& c : capabilities) {
        if (records.size() >= kWebUiLimit) break;
        records.push_back(CapabilityRecord{c.path, c.domain, c.attribute, c.strategy_id,
                                           c.raw_value, c.writable && c.optimization_supported});
    }
    return records;
}

void sleepAndBackoff(std::chrono::seconds& delay) {
    std::this_thread::sleep_for(delay);
    delay = std::min(delay * 2, kMaximumIdle);
}

const char* dispositionName(Disposition d) {
    switch (d) {
        case Disposition::Retain: return "retain";
        case Disposition::Rollback: return "rollback";
        case Disposition::Unresolved: return "unresolved";
    }
    return "unresolved";
}

} // namespace

AutonomousEngine::AutonomousEngine() : state_(defaultStateDirectory()) {
    status_.catalog_status = std::getenv("COREFLOW_STRATEGY_CATALOG") ? "configured" : "not-configured";
    if (!state_.ready()) {
        std::clog << "[CoreFlowEngine] WebUI state directory unavailable: " << state_.directory()
                  << "; the engine continues without the console bridge\n";
    }
}

void AutonomousEngine::applyCommand(Command command) {
    switch (command) {
        case Command::Pause:
            paused_ = true;
            std::clog << "[CoreFlowEngine] Operator command: pause; observation continues, mutation stopped\n";
            break;
        case Command::Resume:
            paused_ = false;
            std::clog << "[CoreFlowEngine] Operator command: resume\n";
            break;
        case Command::Rescan:
            force_discovery_ = true;
            break;
        case Command::ResetLearning:
            optimizer_.reset();
            std::clog << "[CoreFlowEngine] Operator command: learned feedback reset\n";
            break;
        case Command::None:
            break;
    }
}

void AutonomousEngine::publish() {
    status_.generated_unix = unixNow();
    status_.uptime_s = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - started_).count());
    status_.engine_state = paused_ ? "paused" : "running";
    status_.stale_after_s = static_cast<std::uint64_t>(kStaleAfter.count());
    state_.writeStatus(status_);
}

void AutonomousEngine::recordDecision(const ChangeProposal& candidate,
                                      const std::string& disposition, const std::string& detail) {
    JournalRecord record{"decision", unixNow(), candidate.capability_id, candidate.previous_value,
                         candidate.target_value, disposition, detail};
    state_.appendJournal(record);
}

void AutonomousEngine::recordMutation(const ChangeProposal& candidate,
                                      const std::string& disposition, const std::string& detail) {
    JournalRecord record{"mutation", unixNow(), candidate.capability_id, candidate.previous_value,
                         candidate.target_value, disposition, detail};
    state_.appendJournal(record);
}

void AutonomousEngine::run() {
    std::vector<Capability> capabilities;
    bool device_context_logged = false;
    auto last_discovery = Clock::time_point::min();
    auto idle_delay = kMinimumIdle;
    std::size_t previous_count = 0;
    publish();

    for (;;) {
        try {
            setStage("command");
            applyCommand(state_.takeCommand());
            status_.cycles += 1;

            // Consume the optimizer's request exactly once per active cycle, so a retained
            // governor/scheduler change always triggers a fresh inventory.
            const bool optimizer_requested = optimizer_.consumeRediscoveryRequest();
            setStage("discovery");
            const auto now = Clock::now();
            const bool rediscover = force_discovery_ || optimizer_requested ||
                last_discovery == Clock::time_point::min() ||
                now - last_discovery >= kDiscoveryRefresh;
            if (rediscover) {
                capabilities = discovery_.discover();
                last_discovery = Clock::now();
                force_discovery_ = false;
                status_.discovered = capabilities.size();
                if (capabilities.size() != previous_count) {
                    std::clog << "[CoreFlowEngine] Discovery inventory: "
                              << capabilities.size() << " resources\n";
                    previous_count = capabilities.size();
                }
            }

            setStage("observation");
            const auto before = observer_.observe(capabilities);
            // Clear transient cycle exceptions once a complete observation succeeds;
            // recovery failures remain visible until explicitly resolved.
            if (status_.last_error.rfind("cycle exception:", 0) == 0) {
                status_.last_error.clear();
            }
            auto observed_capabilities = capabilities;
            refreshObservedValues(observed_capabilities, before);
            if (rediscover) state_.writeCapabilities(toRecords(observed_capabilities));
            status_.system = before.system;
            status_.workload_signature = workloadSignature(before.system, before.noise);
            status_.eligible = static_cast<std::size_t>(std::count_if(
                observed_capabilities.begin(), observed_capabilities.end(),
                [](const Capability& c) { return c.writable && c.optimization_supported; }));
            if (!device_context_logged) {
                const auto& device = before.device;
                status_.model = device.model;
                status_.soc = device.soc_model;
                status_.android_release = device.android_release;
                status_.kernel_release = device.kernel_release;
                status_.storage_type = device.storage_type;
                std::clog << "[CoreFlowEngine] Device context: model="
                          << (device.model.empty() ? "unknown" : device.model)
                          << "; SoC=" << (device.soc_model.empty() ? "unknown" : device.soc_model)
                          << "; Android=" << (device.android_release.empty() ? "unknown" : device.android_release)
                          << " (API " << (device.android_api.empty() ? "unknown" : device.android_api) << ")"
                          << "; kernel=" << (device.kernel_release.empty() ? "unknown" : device.kernel_release)
                          << "; RAM_kB=" << device.total_memory_kb
                          << "; online_cpus=" << device.online_cpu_count
                          << "; storage=" << device.storage_type << '\n';
                device_context_logged = true;
            }

            // Pause disables mutation and optimization, but deliberately keeps
            // discovery and telemetry alive so the console remains current.
            if (paused_) {
                setStage("paused");
                publish();
                std::this_thread::sleep_for(kPausedPoll);
                continue;
            }

            setStage("optimization");
            const auto candidate = optimizer_.propose(observed_capabilities, before);
            if (!candidate) {
                setStage("idle");
                publish();
                sleepAndBackoff(idle_delay);
                continue;
            }

            setStage("justification");
            std::string reason;
            bool accepted = justifier_.justified(*candidate, before, reason);
            if (accepted) {
                setStage("validation");
                accepted = validator_.validate(*candidate, reason);
            }
            if (!accepted) {
                optimizer_.defer(*candidate, before);
                status_.deferred += 1;
                recordDecision(*candidate, "deferred", reason);
                std::clog << "[CoreFlowEngine] Candidate deferred: " << reason << '\n';
                publish();
                sleepAndBackoff(idle_delay);
                continue;
            }

            idle_delay = kMinimumIdle;
            setStage("mutation");
            const auto applied = mutation_.apply(*candidate);
            if (!applied.write_succeeded) {
                std::string recovery_detail;
                const bool restored = mutation_.rollback(applied, recovery_detail);
                optimizer_.defer(*candidate, before);
                status_.deferred += 1;
                if (!restored) {
                    ++status_.unresolved;
                    status_.last_error = "mutation recovery unresolved: " + recovery_detail;
                }
                const std::string detail = "write or read-back failed; " + recovery_detail;
                recordMutation(*candidate, restored ? "write-failed" : "unresolved", detail);
                std::clog << "[CoreFlowEngine] Mutation failed or read-back differed: "
                          << candidate->path << "; recovery=" << (restored ? "verified" : "unresolved")
                          << "; " << recovery_detail << '\n';
                publish();
                sleepAndBackoff(idle_delay);
                continue;
            }

            // Armed from here: an exception during settle, observe or verify still
            // triggers a best-effort rollback from the destructor.
            internal::RollbackGuard rollback_guard(
                [this](const AppliedChange& change, std::string& reason) {
                    try {
                        const bool restored = mutation_.rollback(change, reason);
                        if (restored) {
                            ++status_.rolled_back;
                        } else {
                            ++status_.unresolved;
                            status_.last_error = "rollback unresolved (exception): " + reason;
                        }
                        return restored;
                    } catch (const std::exception& e) {
                        ++status_.unresolved;
                        reason = e.what();
                        status_.last_error = "rollback unresolved (exception): " + reason;
                        return false;
                    } catch (...) {
                        ++status_.unresolved;
                        reason = "unknown exception during rollback";
                        status_.last_error = "rollback unresolved (exception): " + reason;
                        return false;
                    }
                }, applied);
            setStage("settling");
            publish();
            std::this_thread::sleep_for(kOutcomeSettle);
            setStage("observation");
            const auto after = observer_.observe(capabilities);
            setStage("outcome");
            const auto outcome = verifier_.verify(before.system, after.system, *candidate, before.noise);
            const auto disposition = policy_.decide(outcome);
            auto effective_disposition = disposition;
            std::string detail = outcome.explanation;
            if (disposition == Disposition::Retain) {
                // The measured outcome has committed this mutation. Ancillary
                // learning/journal failures must not roll it back after feedback
                // has already accepted it.
                rollback_guard.dismiss();
                ++status_.retained;
            } else {
                setStage("rollback");
                std::string rollback_detail;
                const bool restored = mutation_.rollback(applied, rollback_detail);
                if (restored) {
                    // Count an actual, verified restoration, including when the
                    // measured outcome was inconclusive and policy chose safety.
                    ++status_.rolled_back;
                    effective_disposition = Disposition::Rollback;
                    detail += "; rollback verified: " + rollback_detail;
                } else {
                    ++status_.unresolved;
                    effective_disposition = Disposition::Unresolved;
                    detail += "; rollback failed: " + rollback_detail;
                    status_.last_error = "rollback unresolved: " + rollback_detail;
                }
                // The explicit attempt has a known result. Do not let the RAII
                // guard silently retry it if later journaling/learning throws.
                rollback_guard.dismiss();
            }
            setStage("learning");
            // Keep the policy disposition for learning: inconclusive outcomes must
            // not train the optimizer, even if the safety rollback was verified.
            optimizer_.recordOutcome(*candidate, outcome, disposition, before);
            journal_.record(*candidate, outcome, effective_disposition, detail);
            recordMutation(*candidate, dispositionName(effective_disposition), detail);
            idle_delay = kMinimumIdle * 2;
            publish();
            sleepAndBackoff(idle_delay);
        } catch (const std::exception& e) {
            const std::string recovery_error = status_.last_error;
            const bool recovery_unresolved =
                recovery_error.rfind("rollback unresolved", 0) == 0 ||
                recovery_error.rfind("mutation recovery unresolved", 0) == 0;
            if (recovery_unresolved) {
                status_.last_error = recovery_error + "; cycle exception: " + e.what();
            } else {
                status_.last_error = std::string("cycle exception: ") + e.what();
            }
            std::clog << "[CoreFlowEngine] cycle exception: " << e.what() << '\n';
            publish();
            sleepAndBackoff(idle_delay);
        } catch (...) {
            if (status_.last_error.rfind("rollback unresolved", 0) == 0 ||
                status_.last_error.rfind("mutation recovery unresolved", 0) == 0) {
                status_.last_error += "; unknown cycle exception";
            } else {
                status_.last_error = "unknown cycle exception";
            }
            std::clog << "[CoreFlowEngine] unknown cycle exception\n";
            publish();
            sleepAndBackoff(idle_delay);
        }
    }
}

} // namespace coreflow
