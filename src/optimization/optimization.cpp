#include "coreflow/optimization/optimization.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>

namespace coreflow {
namespace {
double representationQuantum(const std::string& raw) {
    const auto dot = raw.find('.');
    if (dot == std::string::npos) return 1.0;
    std::size_t decimals = 0;
    for (std::size_t i = dot + 1; i < raw.size() && raw[i] >= '0' && raw[i] <= '9'; ++i)
        ++decimals;
    return std::pow(10.0, -static_cast<double>(decimals));
}
std::string formatNumber(double value, const std::string& raw) {
    const double quantum = representationQuantum(raw);
    if (quantum >= 1.0) {
        std::ostringstream out;
        out << static_cast<long long>(std::llround(value));
        return out.str();
    }
    std::ostringstream out;
    out << std::fixed << std::setprecision(static_cast<int>(std::ceil(-std::log10(quantum)))) << value;
    return out.str();
}
int pressureDirection(const Capability& c, const ObservationFrame& f) {
    // Select a first direction from observed operating pressure where the
    // relationship is broadly meaningful; after evaluation, feedback owns it.
    if (c.domain == "proc_sys.vm" && c.attribute == "swappiness" && f.system.memory_valid) {
        if (f.system.memory_available_ratio < 0.20 ||
            (f.system.memory_pressure_valid && f.system.memory_pressure_some_avg10 > 1.0)) return 1;
        if (f.system.memory_available_ratio > 0.45 &&
            (!f.system.memory_pressure_valid || f.system.memory_pressure_some_avg10 < 0.5)) return -1;
    }
    if ((c.domain == "sysfs.cpu.cpufreq" || c.domain == "sysfs.devfreq") &&
        f.system.temperature_valid && f.system.maximum_temperature_c >= 42.0) return -1;
    if ((c.domain == "sysfs.block.queue") && f.system.io_pressure_valid &&
        f.system.io_pressure_some_avg10 > 1.0) return 1;
    return 0;
}
double candidatePriority(const Capability& c, const ObservationFrame& f) {
    double priority = c.choices.size() > 1 ? 1.0 : 0.0;
    if (f.device.total_memory_kb > 0 && f.device.total_memory_kb < 4ULL * 1024ULL * 1024ULL &&
        c.domain == "proc_sys.vm") priority += 0.5;
    if ((f.device.storage_type == "UFS" || f.device.storage_type == "eMMC") &&
        c.domain == "sysfs.block.queue" && f.system.io_pressure_valid &&
        f.system.io_pressure_some_avg10 > 0.5) priority += 1.0;
    if (f.device.online_cpu_count > 0 && c.domain == "sysfs.cpu.cpufreq" &&
        f.system.cpu_pressure_valid && f.system.cpu_pressure_some_avg10 > 0.5) priority += 0.5;
    if (c.domain == "proc_sys.vm") {
        if (f.system.memory_pressure_valid) priority += std::min(4.0, f.system.memory_pressure_some_avg10);
        if (f.system.memory_valid && f.system.memory_available_ratio < 0.20) priority += 3.0;
    } else if (c.domain == "sysfs.block.queue") {
        if (f.system.io_pressure_valid) priority += std::min(4.0, f.system.io_pressure_some_avg10);
    } else if (c.domain == "sysfs.cpu.cpufreq" || c.domain == "sysfs.devfreq") {
        if (f.system.cpu_pressure_valid) priority += std::min(3.0, f.system.cpu_pressure_some_avg10);
        if (f.system.temperature_valid && f.system.maximum_temperature_c >= 42.0) priority += 3.0;
    }
    return priority;
}
}

std::optional<ChangeProposal> OptimizationEngine::propose(
    const std::vector<Capability>& capabilities, const ObservationFrame& frame) {
    if (capabilities.empty()) return std::nullopt;

    const std::size_t count = capabilities.size();
    std::optional<ChangeProposal> best;
    double best_priority = -std::numeric_limits<double>::infinity();
    std::size_t best_index = next_index_ % count;

    for (std::size_t offset = 0; offset < count; ++offset) {
        const std::size_t index = (next_index_ + offset) % count;
        const auto& c = capabilities[index];
        if (c.id.empty() || c.path.empty() || !c.writable || !c.optimization_supported) continue;
        if (dependency_followup_pending_ && c.domain == dependency_domain_ &&
            c.attribute == dependency_trigger_attribute_) continue;

        ChangeProposal proposal;
        if (!c.choices.empty()) {
            if (c.choices.size() < 2 ||
                std::find(c.choices.begin(), c.choices.end(), c.raw_value) == c.choices.end()) continue;
            std::size_t pick = next_choice_index_.count(c.id) ? next_choice_index_[c.id] : 0;
            bool found = false;
            for (std::size_t attempt = 0; attempt < c.choices.size(); ++attempt) {
                const auto candidate_index = (pick + attempt) % c.choices.size();
                if (c.choices[candidate_index] == c.raw_value) continue;
                proposal = ChangeProposal{c.id, c.path, c.raw_value, c.choices[candidate_index],
                    0.0, 0.0, "Kernel-advertised discrete policy candidate; evaluate runtime outcome",
                    c.choices};
                next_choice_index_[c.id] = (candidate_index + 1) % c.choices.size();
                found = true;
                break;
            }
            if (!found) continue;
        } else {
            if (!c.numeric || !c.range.valid ||
                !(std::isfinite(c.numeric_value) && std::isfinite(c.range.minimum) &&
                  std::isfinite(c.range.maximum) && c.range.minimum < c.range.maximum &&
                  c.range.minimum <= c.numeric_value && c.numeric_value <= c.range.maximum)) continue;

            const double quantum = representationQuantum(c.raw_value);
            const double span = c.range.maximum - c.range.minimum;
            if (!(quantum > 0.0) || !std::isfinite(quantum) || !(span > 0.0)) continue;
            // Begin with a small perturbation derived from the discovered interval
            // and then adapt the fraction from observed outcomes. No path-specific
            // deltas or fixed candidate/resource count are used.
            const auto learned_step = step_fraction_by_capability_.find(c.id);
            const double fraction = learned_step == step_fraction_by_capability_.end()
                ? 0.01 : learned_step->second;
            double step = std::max(quantum, span * fraction);
            if (quantum >= 1.0) step = std::max(1.0, std::round(step));
            int direction = pressureDirection(c, frame);
            const auto learned = direction_by_capability_.find(c.id);
            if (direction == 0) direction = learned == direction_by_capability_.end() ? 1 : learned->second;
            double target = c.numeric_value + static_cast<double>(direction) * step;
            if (target > c.range.maximum || target < c.range.minimum) {
                direction = -direction;
                target = c.numeric_value + static_cast<double>(direction) * step;
            }
            target = std::clamp(target, c.range.minimum, c.range.maximum);
            if (quantum >= 1.0) target = std::round(target);
            if (!std::isfinite(target) || target == c.numeric_value) continue;
            proposal = ChangeProposal{c.id, c.path, c.raw_value, formatNumber(target, c.raw_value),
                c.range.minimum, c.range.maximum,
                "Capability-derived bounded candidate; direction adapted from runtime pressure and outcome feedback", {}};
            direction_by_capability_.try_emplace(c.id, direction);
        }

        double priority = candidatePriority(c, frame);
        if (dependency_followup_pending_ && c.domain == dependency_domain_ &&
            c.attribute != dependency_trigger_attribute_) priority += 5.0;
        // A retained governor/scheduler change is followed by a related control
        // in the same discovered subsystem when one is eligible. The relationship
        // is domain-derived; no fixed list of per-device paths is required.
        if (!best || priority > best_priority) {
            best = std::move(proposal);
            best_priority = priority;
            best_index = index;
        }
    }
    if (best) {
        next_index_ = (best_index + 1) % count;
        if (dependency_followup_pending_) dependency_followup_pending_ = false;
    }
    return best;
}

void OptimizationEngine::recordOutcome(const std::string& id, bool improved) {
    if (id.empty()) return;
    auto direction = direction_by_capability_.find(id);
    if (direction == direction_by_capability_.end()) {
        direction_by_capability_.emplace(id, improved ? 1 : -1);
    } else if (!improved) {
        direction->second = -direction->second;
    }

    auto step = step_fraction_by_capability_.find(id);
    if (step == step_fraction_by_capability_.end()) {
        step_fraction_by_capability_.emplace(id, 0.01);
        step = step_fraction_by_capability_.find(id);
    }
    // A beneficial change is refined with smaller steps. A neutral or regressing
    // measurement reverses the search direction and broadens the next bounded
    // probe. The safety validator still limits every target to its live range.
    step->second = improved ? std::max(0.0001, step->second * 0.5)
                            : std::min(0.5, step->second * 2.0);
}

void OptimizationEngine::recordOutcome(const std::string& id, const std::string& domain,
                                       const std::string& attribute, bool improved, bool retained) {
    recordOutcome(id, improved);
    if (!retained || domain.empty() || attribute.empty()) return;
    if (attribute == "scheduler" || attribute == "scaling_governor" ||
        attribute == "governor" || attribute == "energy_performance_preference") {
        dependency_domain_ = domain;
        dependency_trigger_attribute_ = attribute;
        dependency_followup_pending_ = true;
    }
}
}
