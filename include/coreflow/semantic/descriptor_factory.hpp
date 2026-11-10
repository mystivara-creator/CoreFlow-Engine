#pragma once

#include "coreflow/semantic/dynamic_semantic_descriptor.hpp"

#include <optional>
#include <utility>

namespace coreflow::semantic {

class CapabilityResolver;

class DescriptorFactory final {
private:
    friend class CapabilityResolver;
    [[nodiscard]] static std::optional<DynamicSemanticDescriptor> issue(
        DescriptorDraft&& draft, std::uint64_t active_generation, std::uint64_t now_ns) noexcept {
        if (draft.generation != active_generation || !draft.capability.isValid() ||
            !draft.semantic.role.isValid() || !draft.semantic.domain.isValid() ||
            !draft.semantic.vocabulary.isValid() || !draft.provenance.id.isValid() ||
            !draft.provenance.provider.isValid() || draft.provenance.generation != active_generation ||
            draft.provenance.expires_at_ns <= now_ns || !draft.value.numeric.isValid() ||
            draft.semantic.confidence < 0.0F || draft.semantic.confidence > 1.0F ||
            draft.risk.estimated_risk < 0.0F || draft.risk.estimated_risk > 1.0F)
            return std::nullopt;

        for (const auto& item : draft.evidence) {
            if (!item.id.isValid() || item.generation != active_generation ||
                item.provenance != draft.provenance.id || item.observed_at_ns > now_ns)
                return std::nullopt;
        }
        return DynamicSemanticDescriptor(std::move(draft), DescriptorState::Validated);
    }
};

class CapabilityResolver final {
public:
    [[nodiscard]] std::optional<DynamicSemanticDescriptor> validateAndIssue(
        DescriptorDraft&& draft, std::uint64_t active_generation, std::uint64_t now_ns) const noexcept {
        return DescriptorFactory::issue(std::move(draft), active_generation, now_ns);
    }
};

} // namespace coreflow::semantic
