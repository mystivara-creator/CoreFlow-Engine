#include "coreflow/runtime/engine.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <unordered_map>

namespace coreflow {
namespace {
using Clock = std::chrono::steady_clock;
constexpr auto kMinimumIdle = std::chrono::seconds(5);
constexpr auto kMaximumIdle = std::chrono::seconds(60);
constexpr auto kDiscoveryRefresh = std::chrono::minutes(2);
constexpr auto kOutcomeSettle = std::chrono::seconds(10);

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
        if (found == values.end()) continue;
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
void sleepAndBackoff(std::chrono::seconds& delay) {
    std::this_thread::sleep_for(delay);
    delay = std::min(delay * 2, kMaximumIdle);
}
}

void AutonomousEngine::run() {
    std::vector<Capability> capabilities;
    bool device_context_logged = false;
    auto last_discovery = Clock::time_point::min();
    auto idle_delay = kMinimumIdle;
    std::size_t previous_count = 0;

    for (;;) {
        try {
            const auto now = Clock::now();
            if (last_discovery == Clock::time_point::min() ||
                now - last_discovery >= kDiscoveryRefresh) {
                capabilities = discovery_.discover();
                last_discovery = Clock::now();
                if (capabilities.size() != previous_count) {
                    std::clog << "[CoreFlowEngine] Discovery inventory: "
                              << capabilities.size() << " resources\n";
                    previous_count = capabilities.size();
                }
            }

            const auto before = observer_.observe(capabilities);
            refreshObservedValues(capabilities, before);
            if (!device_context_logged) {
                const auto& device = before.device;
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
            const auto candidate = optimizer_.propose(capabilities, before);
            if (!candidate) {
                sleepAndBackoff(idle_delay);
                continue;
            }

            std::string reason;
            if (!justifier_.justified(*candidate, before, reason) ||
                !validator_.validate(*candidate, reason)) {
                std::clog << "[CoreFlowEngine] Candidate skipped: " << reason << '\n';
                sleepAndBackoff(idle_delay);
                continue;
            }

            idle_delay = kMinimumIdle;
            const auto applied = mutation_.apply(*candidate);
            if (!applied.write_succeeded) {
                std::string recovery_detail;
                const bool restored = mutation_.rollback(applied, recovery_detail);
                std::clog << "[CoreFlowEngine] Mutation failed or read-back differed: "
                          << candidate->path << "; recovery="
                          << (restored ? "verified" : "unresolved")
                          << "; " << recovery_detail << '\n';
                sleepAndBackoff(idle_delay);
                continue;
            }

            std::this_thread::sleep_for(kOutcomeSettle);
            const auto after = observer_.observe(capabilities);
            const auto outcome = verifier_.verify(before.system, after.system);
            const auto disposition = policy_.decide(outcome);
            std::string detail = outcome.explanation;
            if (disposition != Disposition::Retain) {
                if (!mutation_.rollback(applied, detail)) detail += "; rollback unresolved";
            }
            if (outcome.measurable) {
                const auto cap = std::find_if(capabilities.begin(), capabilities.end(),
                    [&](const Capability& c) { return c.id == candidate->capability_id; });
                if (cap != capabilities.end()) {
                    optimizer_.recordOutcome(candidate->capability_id, cap->domain, cap->attribute,
                        disposition == Disposition::Retain, disposition == Disposition::Retain);
                } else {
                    optimizer_.recordOutcome(candidate->capability_id,
                                             disposition == Disposition::Retain);
                }
            }
            journal_.record(*candidate, outcome, disposition, detail);
            idle_delay = kMinimumIdle * 2;
            sleepAndBackoff(idle_delay);
        } catch (const std::exception& e) {
            std::clog << "[CoreFlowEngine] cycle exception: " << e.what() << '\n';
            sleepAndBackoff(idle_delay);
        } catch (...) {
            std::clog << "[CoreFlowEngine] unknown cycle exception\n";
            sleepAndBackoff(idle_delay);
        }
    }
}
}
