#pragma once

#include "coreflow/semantic/semantic_types.hpp"

#include <cstdint>
#include <span>

namespace coreflow::semantic {

enum class DomainEvidenceKind : std::uint8_t {
    SubsystemAssociation, DriverAbi, PsiResponseCorrelation
};
enum class DomainAssessmentState : std::uint8_t { Unresolved, Candidate, Corroborated, Verified };

struct DomainRelationshipEvidence final {
    DomainEvidenceKind kind{DomainEvidenceKind::SubsystemAssociation};
    FunctionalDomain domain{FunctionalDomain::Unresolved};
    EvidenceId evidence{};
    ProvenanceId provenance{};
    Identifier128 relationship{};
    std::uint64_t generation{0};
    std::uint64_t observed_at_ns{0};
    bool positive{false};
    // Binds this relation set to the exact discovered capability node.
    CapabilityId capability{};
};

struct FunctionalDomainAssessment final {
    FunctionalDomain domain{FunctionalDomain::Unresolved};
    DomainAssessmentState state{DomainAssessmentState::Unresolved};
    std::uint64_t generation{0};
    std::uint64_t assessed_at_ns{0};
    std::uint8_t corroborating_relations{0};
    float confidence{0.0F};

    [[nodiscard]] constexpr bool mutationEligible() const noexcept {
        return state == DomainAssessmentState::Verified &&
               domain != FunctionalDomain::Unresolved &&
               corroborating_relations == 3U;
    }
};

class DomainEvidenceAggregator final {
public:
    // Requires independent, fresh runtime relations for all three channels.
    // Any stale, negative, duplicate, or cross-domain evidence fails closed.
    [[nodiscard]] FunctionalDomainAssessment aggregate(
        std::span<const DomainRelationshipEvidence> evidence,
        std::uint64_t active_generation,
        std::uint64_t now_ns,
        std::uint64_t maximum_age_ns) const noexcept;
};

} // namespace coreflow::semantic
