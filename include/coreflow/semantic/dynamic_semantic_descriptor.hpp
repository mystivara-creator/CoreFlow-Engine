#pragma once

#include "coreflow/semantic/semantic_types.hpp"

#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace coreflow::semantic {

struct CapabilityProvenance final {
    ProvenanceId id{};
    Identifier128 provider{};
    std::uint64_t generation{0};
    std::uint64_t observed_at_ns{0};
    std::uint64_t expires_at_ns{0};
};

struct CapabilityEvidence final {
    EvidenceId id{};
    ProvenanceId provenance{};
    VocabularyId evidence_kind{};
    EvidenceStrength strength{EvidenceStrength::None};
    ObservationId observation{};
    std::uint64_t generation{0};
    std::uint64_t observed_at_ns{0};
};

struct SemanticBinding final {
    SemanticRoleId role{};
    SemanticDomainId domain{};
    VocabularyId vocabulary{};
    float confidence{0.0F};
};

struct NumericConstraints final {
    bool has_minimum{false};
    bool has_maximum{false};
    bool has_resolution{false};
    std::int64_t minimum{0};
    std::int64_t maximum{0};
    std::int64_t resolution{0};

    [[nodiscard]] constexpr bool isValid() const noexcept {
        if (has_minimum && has_maximum && minimum > maximum) return false;
        if (has_resolution && resolution <= 0) return false;
        return true;
    }
};

struct ValueModel final {
    ValueKind kind{ValueKind::Unknown};
    UnitId unit{};
    NumericConstraints numeric{};
    bool supports_readback{false};
    bool supports_enumeration{false};
};

struct MutationSemantics final {
    MutationMode mode{MutationMode::None};
    bool reversible{false};
    bool requires_readback{true};
    bool requires_exclusive_ownership{true};
};

struct CapabilityRisk final {
    RiskClassId risk_class{};
    float estimated_risk{1.0F};
    float confidence{0.0F};
};

struct DescriptorDraft final {
    CapabilityId capability{};
    SemanticBinding semantic{};
    ValueModel value{};
    MutationSemantics mutation{};
    AccessCapability access{AccessCapability::None};
    CapabilityRisk risk{};
    CapabilityProvenance provenance{};
    std::vector<CapabilityEvidence> evidence{};
    std::uint64_t generation{0};
};

class DynamicSemanticDescriptor final {
public:
    DynamicSemanticDescriptor(const DynamicSemanticDescriptor&) = default;
    DynamicSemanticDescriptor(DynamicSemanticDescriptor&&) noexcept = default;
    DynamicSemanticDescriptor& operator=(const DynamicSemanticDescriptor&) = delete;
    DynamicSemanticDescriptor& operator=(DynamicSemanticDescriptor&&) = delete;

    [[nodiscard]] CapabilityId capabilityId() const noexcept { return capability_; }
    [[nodiscard]] const SemanticBinding& semantic() const noexcept { return semantic_; }
    [[nodiscard]] const ValueModel& value() const noexcept { return value_; }
    [[nodiscard]] const MutationSemantics& mutation() const noexcept { return mutation_; }
    [[nodiscard]] AccessCapability access() const noexcept { return access_; }
    [[nodiscard]] const CapabilityRisk& risk() const noexcept { return risk_; }
    [[nodiscard]] const CapabilityProvenance& provenance() const noexcept { return provenance_; }
    [[nodiscard]] std::span<const CapabilityEvidence> evidence() const noexcept { return evidence_; }
    [[nodiscard]] std::uint64_t generation() const noexcept { return generation_; }
    [[nodiscard]] DescriptorState state() const noexcept { return state_; }

    [[nodiscard]] bool hasWriteAccess() const noexcept { return hasAccess(access_, AccessCapability::Write); }
    [[nodiscard]] bool isResolved() const noexcept {
        return state_ == DescriptorState::Resolved || state_ == DescriptorState::Validated;
    }
    [[nodiscard]] bool isMutationCapable() const noexcept {
        if (state_ != DescriptorState::Validated || !hasWriteAccess() || mutation_.mode == MutationMode::None)
            return false;
        if (!capability_.isValid() || !semantic_.role.isValid() || !semantic_.domain.isValid() ||
            !semantic_.vocabulary.isValid() || !provenance_.id.isValid() ||
            !provenance_.provider.isValid() || !risk_.risk_class.isValid() ||
            value_.kind == ValueKind::Unknown || !value_.numeric.isValid() ||
            (mutation_.requires_readback && !value_.supports_readback)) return false;
        for (const auto& item : evidence_) {
            if (item.generation == generation_ && item.provenance == provenance_.id &&
                item.strength != EvidenceStrength::None && item.strength != EvidenceStrength::Weak)
                return true;
        }
        return false;
    }

    [[nodiscard]] bool isMutationCapable(std::uint64_t active_generation,
                                         std::uint64_t now_ns) const noexcept {
        return generation_ == active_generation && provenance_.expires_at_ns > now_ns &&
               isMutationCapable();
    }

private:
    friend class DescriptorFactory;
    DynamicSemanticDescriptor(DescriptorDraft&& draft, DescriptorState state) noexcept
        : capability_(draft.capability), semantic_(draft.semantic), value_(draft.value),
          mutation_(draft.mutation), access_(draft.access), risk_(draft.risk),
          provenance_(draft.provenance), evidence_(std::move(draft.evidence)),
          generation_(draft.generation), state_(state) {}

    const CapabilityId capability_;
    const SemanticBinding semantic_;
    const ValueModel value_;
    const MutationSemantics mutation_;
    const AccessCapability access_;
    const CapabilityRisk risk_;
    const CapabilityProvenance provenance_;
    const std::vector<CapabilityEvidence> evidence_;
    const std::uint64_t generation_;
    const DescriptorState state_;
};

} // namespace coreflow::semantic
