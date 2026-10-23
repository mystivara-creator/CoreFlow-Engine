#include "coreflow/optimization/optimization.hpp"
#include "coreflow/optimization/workload.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
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
    std::ostringstream out;
    if (quantum >= 1.0) {
        // Avoid llround() for a device-exposed value outside long long range.
        out << std::fixed << std::setprecision(0) << value;
    } else {
        const auto precision = static_cast<int>(std::ceil(-std::log10(quantum)));
        out << std::fixed << std::setprecision(std::clamp(precision, 0, 12)) << value;
    }
    return out.str();
}

std::string contextKey(const std::string& capability_id, const std::string& signature) {
    return capability_id + '\x1f' + signature;
}

bool isSelector(const std::string& domain, const std::string& attribute) {
    return (domain == "sysfs.block.queue" && attribute == "scheduler") ||
           (domain == "sysfs.cpu.cpufreq" &&
            (attribute == "scaling_governor" || attribute == "energy_performance_preference")) ||
           (domain == "sysfs.devfreq" && attribute == "governor");
}

bool isKernelDiscreteControl(const std::string& domain, const std::string& attribute) {
    if (isSelector(domain, attribute)) return true;
    return (domain == "sysfs.cpu.cpufreq" &&
            (attribute == "scaling_min_freq" || attribute == "scaling_max_freq")) ||
           (domain == "sysfs.devfreq" &&
            (attribute == "min_freq" || attribute == "max_freq"));
}

bool validNumericCapability(const Capability& capability) {
    return capability.numeric && capability.range.valid &&
           std::isfinite(capability.numeric_value) &&
           std::isfinite(capability.range.minimum) &&
           std::isfinite(capability.range.maximum) &&
           capability.range.minimum < capability.range.maximum &&
           capability.range.minimum <= capability.numeric_value &&
           capability.numeric_value <= capability.range.maximum;
}

} // namespace

void OptimizationEngine::trackContext(const std::string& key) {
    if (key.empty() || tracked_context_keys_.find(key) != tracked_context_keys_.end()) return;

    // Insert into the eviction queue first, then the membership set. If either
    // allocation throws, keep the two structures consistent for future eviction.
    context_history_order_.push_back(key);
    try {
        const auto [unused, inserted] = tracked_context_keys_.insert(key);
        (void)unused;
        if (!inserted) {
            context_history_order_.pop_back();
            return;
        }
    } catch (...) {
        context_history_order_.pop_back();
        throw;
    }

    constexpr std::size_t kMaximumTrackedContexts = 512;
    while (context_history_order_.size() > kMaximumTrackedContexts) {
        std::string expired = std::move(context_history_order_.front());
        context_history_order_.pop_front();
        tracked_context_keys_.erase(expired);
        tried_choices_by_context_.erase(expired);
        retained_choice_contexts_.erase(expired);
        deferred_numeric_contexts_.erase(expired);
        non_improving_by_context_.erase(expired);
        numeric_trials_by_context_.erase(expired);
        direction_by_context_.erase(expired);
        step_fraction_by_context_.erase(expired);
    }
}

void OptimizationEngine::trackCapability(const std::string& capability_id) {
    if (capability_id.empty() ||
        tracked_capability_ids_.find(capability_id) != tracked_capability_ids_.end()) return;

    capability_history_order_.push_back(capability_id);
    try {
        const auto [unused, inserted] = tracked_capability_ids_.insert(capability_id);
        (void)unused;
        if (!inserted) {
            capability_history_order_.pop_back();
            return;
        }
    } catch (...) {
        capability_history_order_.pop_back();
        throw;
    }

    constexpr std::size_t kMaximumTrackedCapabilities = 512;
    while (capability_history_order_.size() > kMaximumTrackedCapabilities) {
        std::string expired = std::move(capability_history_order_.front());
        capability_history_order_.pop_front();
        tracked_capability_ids_.erase(expired);
        next_choice_index_.erase(expired);
        legacy_direction_by_capability_.erase(expired);
        legacy_step_fraction_by_capability_.erase(expired);
    }
}

std::optional<ChangeProposal> OptimizationEngine::propose(
    const std::vector<Capability>& capabilities, const ObservationFrame& frame) {
    if (capabilities.empty()) return std::nullopt;

    const std::string signature = workloadSignature(frame.system);
    if (signature == "idle") return std::nullopt;

    const bool has_dependency = dependency_followup_pending_ &&
        dependency_workload_signature_ == signature;
    std::optional<ChangeProposal> best;
    double best_priority = -std::numeric_limits<double>::infinity();
    std::size_t best_index = next_index_ % capabilities.size();

    for (std::size_t offset = 0; offset < capabilities.size(); ++offset) {
        const std::size_t index = (next_index_ + offset) % capabilities.size();
        const auto& capability = capabilities[index];
        if (capability.id.empty() || capability.path.empty() || !capability.writable ||
            !capability.optimization_supported) continue;

        const double workload_score = workloadScoreForDomain(capability.domain, frame.system);
        if (!(workload_score > 0.0) || !std::isfinite(workload_score)) continue;

        const std::string parent = std::filesystem::path(capability.path).parent_path().string();
        if (has_dependency) {
            // A retained scheduler/governor change is followed only by a control
            // in the exact same queue or CPU policy, not any resource in a broad
            // subsystem. This preserves per-device kernel relationships.
            if (capability.domain != dependency_domain_ || parent != dependency_parent_ ||
                capability.attribute == dependency_trigger_attribute_) continue;
        } else if (dependency_followup_pending_) {
            dependency_followup_pending_ = false;
        }

        ChangeProposal proposal;
        if (!capability.choices.empty()) {
            if (capability.choices.size() < 2 ||
                std::find(capability.choices.begin(), capability.choices.end(), capability.raw_value) ==
                    capability.choices.end() ||
                !isKernelDiscreteControl(capability.domain, capability.attribute)) continue;

            const std::string key = contextKey(capability.id, signature);
            // Once a kernel-advertised choice has produced a measurable retained
            // result for this workload, stop cycling it. When the workload changes,
            // its context is different and can be evaluated independently.
            if (retained_choice_contexts_.count(key) != 0) continue;
            const auto tried_it = tried_choices_by_context_.find(key);
            const auto found = next_choice_index_.find(capability.id);
            const std::size_t start = found == next_choice_index_.end() ? 0 : found->second;
            bool selected = false;
            for (std::size_t attempt = 0; attempt < capability.choices.size(); ++attempt) {
                const auto candidate_index = (start + attempt) % capability.choices.size();
                const auto& value = capability.choices[candidate_index];
                const bool already_tried = tried_it != tried_choices_by_context_.end() &&
                    tried_it->second.count(value) != 0;
                if (value == capability.raw_value || already_tried) continue;
                proposal = ChangeProposal{capability.id, capability.path, capability.raw_value, value,
                    0.0, 0.0,
                    "Kernel-advertised choice tested only under matching runtime pressure; retain only after subsystem-specific evidence",
                    capability.choices, capability.domain, capability.attribute};
                selected = true;
                break;
            }
            if (!selected) continue;
        } else {
            const std::string key = contextKey(capability.id, signature);
            if (deferred_numeric_contexts_.count(key) != 0) continue;
            if (!validNumericCapability(capability)) continue;

            const double quantum = representationQuantum(capability.raw_value);
            const double span = capability.range.maximum - capability.range.minimum;
            if (!(quantum > 0.0) || !std::isfinite(quantum) || !(span > 0.0) ||
                !std::isfinite(span)) continue;

            const auto learned_step = step_fraction_by_context_.find(key);
            const double fraction = learned_step == step_fraction_by_context_.end()
                ? 0.01 : learned_step->second;
            double step = std::max(quantum, span * fraction);
            if (quantum >= 1.0) step = std::max(1.0, std::round(step));
            if (!std::isfinite(step) || !(step > 0.0)) continue;

            const auto learned = direction_by_context_.find(key);
            // Start with a generic bounded probe; only measured feedback adapts
            // the direction. No per-parameter preferred values are assumed.
            // Candidate scoring must be side-effect-free: do not persist learning
            // state for a capability unless its proposal is returned or evaluated.
            int direction = learned == direction_by_context_.end() ? 1 : learned->second;
            double target = capability.numeric_value + static_cast<double>(direction) * step;
            if (!std::isfinite(target) || target > capability.range.maximum ||
                target < capability.range.minimum) {
                direction = -direction;
                target = capability.numeric_value + static_cast<double>(direction) * step;
            }
            if (!std::isfinite(target)) continue;
            target = std::clamp(target, capability.range.minimum, capability.range.maximum);
            if (quantum >= 1.0) target = std::round(target);
            if (!std::isfinite(target) || target == capability.numeric_value) continue;

            // Keep coupled frequency controls coherent. If the sibling bound is
            // unavailable or malformed, fail closed instead of proposing a
            // min/max pair that the kernel may reject or interpret unexpectedly.
            const bool is_minimum = capability.attribute == "scaling_min_freq" ||
                                    capability.attribute == "min_freq";
            const bool is_maximum = capability.attribute == "scaling_max_freq" ||
                                    capability.attribute == "max_freq";
            if (is_minimum || is_maximum) {
                const std::string sibling_attribute = is_minimum
                    ? (capability.attribute == "scaling_min_freq" ? "scaling_max_freq" : "max_freq")
                    : (capability.attribute == "scaling_max_freq" ? "scaling_min_freq" : "min_freq");
                const auto parent_path = std::filesystem::path(capability.path).parent_path();
                const auto sibling = std::find_if(capabilities.begin(), capabilities.end(),
                    [&](const Capability& other) {
                        return other.domain == capability.domain &&
                               other.attribute == sibling_attribute &&
                               std::filesystem::path(other.path).parent_path() == parent_path &&
                               other.numeric && std::isfinite(other.numeric_value);
                    });
                if (sibling == capabilities.end()) continue;
                if ((is_minimum && target > sibling->numeric_value) ||
                    (is_maximum && target < sibling->numeric_value)) continue;
            }

            proposal = ChangeProposal{capability.id, capability.path, capability.raw_value,
                formatNumber(target, capability.raw_value), capability.range.minimum,
                capability.range.maximum,
                "Small bounded probe selected from the active subsystem pressure; feedback adapts direction and step size",
                {}, capability.domain, capability.attribute};
        }

        double priority = workload_score;
        if (isSelector(capability.domain, capability.attribute)) priority += 0.25;
        if (has_dependency) priority += 1000.0;
        if (!best || priority > best_priority) {
            best = std::move(proposal);
            best_priority = priority;
            best_index = index;
        }
    }

    if (best) {
        // Exploration state must only advance for the candidate actually returned.
        // Updating it while scoring alternatives would silently consume choices
        // from unrelated governors/queues that were never applied.
        if (!best->allowed_values.empty() && best_index < capabilities.size()) {
            const auto& capability = capabilities[best_index];
            const std::string key = contextKey(best->capability_id, signature);
            trackContext(key);
            tried_choices_by_context_[key].insert(best->target_value);
            const auto selected = std::find(capability.choices.begin(), capability.choices.end(),
                                            best->target_value);
            if (selected != capability.choices.end() && !capability.choices.empty()) {
                const auto selected_index = static_cast<std::size_t>(selected - capability.choices.begin());
                trackCapability(capability.id);
                next_choice_index_[capability.id] = (selected_index + 1) % capability.choices.size();
            }
        }
        next_index_ = (best_index + 1) % capabilities.size();
        if (has_dependency) dependency_followup_pending_ = false;
    } else if (has_dependency) {
        // No semantically related control is eligible. Resume normal policy on
        // the next cycle instead of spinning on an impossible dependency.
        dependency_followup_pending_ = false;
    }
    return best;
}

void OptimizationEngine::recordOutcome(const std::string& id, bool improved) {
    // Compatibility API lacks workload identity, so it must not train the
    // workload-scoped policy used by propose(). Its independent legacy state is
    // still bounded so external callers cannot grow these maps indefinitely.
    if (id.empty()) return;
    trackCapability(id);
    auto direction = legacy_direction_by_capability_.find(id);
    if (direction == legacy_direction_by_capability_.end()) {
        legacy_direction_by_capability_.emplace(id, improved ? 1 : -1);
    } else if (!improved) {
        direction->second = -direction->second;
    }
    auto step = legacy_step_fraction_by_capability_.find(id);
    if (step == legacy_step_fraction_by_capability_.end()) {
        legacy_step_fraction_by_capability_.emplace(id, 0.01);
        step = legacy_step_fraction_by_capability_.find(id);
    }
    step->second = improved ? std::max(0.0001, step->second * 0.5)
                            : std::min(0.5, step->second * 2.0);
}

void OptimizationEngine::recordOutcome(const std::string& id, const std::string& domain,
                                       const std::string& attribute, bool improved, bool retained) {
    recordOutcome(id, improved);
    // Compatibility overload for callers which only provide semantic identity.
    // A path-aware dependency is recorded by the ChangeProposal overload below.
    if (!retained || domain.empty() || attribute.empty()) return;
    if (isSelector(domain, attribute)) {
        dependency_domain_ = domain;
        dependency_trigger_attribute_ = attribute;
        dependency_parent_.clear();
        dependency_workload_signature_.clear();
        dependency_followup_pending_ = false;
    }
}

void OptimizationEngine::recordOutcome(const ChangeProposal& proposal,
                                       const Outcome& outcome,
                                       Disposition disposition,
                                       const ObservationFrame& frame) {
    if (!outcome.measurable || disposition == Disposition::Unresolved) return;
    const bool retained = disposition == Disposition::Retain;
    recordOutcome(proposal, outcome.improved && retained, retained, frame);
}

void OptimizationEngine::recordOutcome(const ChangeProposal& proposal, bool improved,
                                       bool retained, const ObservationFrame& frame) {
    const std::string signature = workloadSignature(frame.system);
    const std::string proposal_key = contextKey(proposal.capability_id, signature);
    trackContext(proposal_key);
    {
        auto direction = direction_by_context_.find(proposal_key);
        if (direction == direction_by_context_.end())
            direction_by_context_.emplace(proposal_key, improved ? 1 : -1);
        else if (!improved)
            direction->second = -direction->second;
        auto step = step_fraction_by_context_.find(proposal_key);
        if (step == step_fraction_by_context_.end()) {
            step_fraction_by_context_.emplace(proposal_key, 0.01);
            step = step_fraction_by_context_.find(proposal_key);
        }
        step->second = improved ? std::max(0.0001, step->second * 0.5)
                                : std::min(0.5, step->second * 2.0);
    }
    deferred_numeric_contexts_.erase(proposal_key);
    if (proposal.allowed_values.empty()) {
        constexpr unsigned kMaximumNumericTrialsPerContext = 6;
        auto& trials = numeric_trials_by_context_[proposal_key];
        if (trials < kMaximumNumericTrialsPerContext) ++trials;
        if (trials >= kMaximumNumericTrialsPerContext)
            deferred_numeric_contexts_.insert(proposal_key);

        if (improved && retained) {
            non_improving_by_context_.erase(proposal_key);
        } else {
            auto& consecutive_misses = non_improving_by_context_[proposal_key];
            if (consecutive_misses < 2) ++consecutive_misses;
            if (consecutive_misses >= 2) deferred_numeric_contexts_.insert(proposal_key);
        }
    }
    if (!proposal.allowed_values.empty()) {
        const std::string key = contextKey(proposal.capability_id, signature);
        if (retained) retained_choice_contexts_.insert(key);
    }
    if (!retained || !isSelector(proposal.domain, proposal.attribute) || signature == "idle") return;

    dependency_domain_ = proposal.domain;
    dependency_trigger_attribute_ = proposal.attribute;
    dependency_parent_ = std::filesystem::path(proposal.path).parent_path().string();
    dependency_workload_signature_ = signature;
    dependency_followup_pending_ = !dependency_parent_.empty();
}

void OptimizationEngine::defer(const ChangeProposal& proposal, const ObservationFrame& frame) {
    if (proposal.capability_id.empty() || !proposal.allowed_values.empty()) return;
    const std::string signature = workloadSignature(frame.system);
    if (signature == "idle") return;
    const auto key = contextKey(proposal.capability_id, signature);
    trackContext(key);
    deferred_numeric_contexts_.insert(key);
    if (dependency_followup_pending_ && dependency_workload_signature_ == signature)
        dependency_followup_pending_ = false;
}

} // namespace coreflow
