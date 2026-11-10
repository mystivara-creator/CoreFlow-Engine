#include "coreflow/semantic/domain_evidence_aggregator.hpp"

namespace coreflow::semantic {

FunctionalDomainAssessment DomainEvidenceAggregator::aggregate(
    std::span<const DomainRelationshipEvidence> evidence,
    std::uint64_t active_generation,
    std::uint64_t now_ns,
    std::uint64_t maximum_age_ns) const noexcept {
    FunctionalDomainAssessment result{};
    result.generation = active_generation;
    if (evidence.empty() || active_generation == 0U || maximum_age_ns == 0U) return result;

    FunctionalDomain selected = FunctionalDomain::Unresolved;
    Identifier128 selected_relationship{};
    CapabilityId selected_capability{};
    ProvenanceId subsystem_provenance{}, driver_provenance{}, psi_provenance{};
    EvidenceId subsystem_evidence{}, driver_evidence{}, psi_evidence{};
    bool seen_subsystem = false;
    bool seen_driver = false;
    bool seen_psi = false;
    for (const auto& item : evidence) {
        if (!item.positive || item.domain == FunctionalDomain::Unresolved ||
            !item.evidence.isValid() || !item.provenance.isValid() ||
            !item.relationship.isValid() || !item.capability.isValid() ||
            item.generation != active_generation || item.observed_at_ns > now_ns ||
            now_ns - item.observed_at_ns > maximum_age_ns) {
            return result;
        }
        if (selected == FunctionalDomain::Unresolved) {
            selected = item.domain;
            selected_relationship = item.relationship;
            selected_capability = item.capability;
        } else if (selected != item.domain || selected_relationship != item.relationship ||
                   selected_capability != item.capability) {
            return result;
        }

        switch (item.kind) {
            case DomainEvidenceKind::SubsystemAssociation:
                if (seen_subsystem) return result;
                seen_subsystem = true;
                subsystem_provenance = item.provenance;
                subsystem_evidence = item.evidence;
                break;
            case DomainEvidenceKind::DriverAbi:
                if (seen_driver) return result;
                seen_driver = true;
                driver_provenance = item.provenance;
                driver_evidence = item.evidence;
                break;
            case DomainEvidenceKind::PsiResponseCorrelation:
                if (seen_psi) return result;
                seen_psi = true;
                psi_provenance = item.provenance;
                psi_evidence = item.evidence;
                break;
            default:
                return result;
        }
    }

    if (seen_subsystem && seen_driver &&
        (subsystem_provenance == driver_provenance || subsystem_evidence == driver_evidence)) return result;
    if (seen_subsystem && seen_psi &&
        (subsystem_provenance == psi_provenance || subsystem_evidence == psi_evidence)) return result;
    if (seen_driver && seen_psi &&
        (driver_provenance == psi_provenance || driver_evidence == psi_evidence)) return result;

    if (!(seen_subsystem && seen_driver && seen_psi)) {
        // Partial evidence cannot assign a functional domain.
        result.state = DomainAssessmentState::Candidate;
        result.corroborating_relations = static_cast<std::uint8_t>(
            static_cast<unsigned>(seen_subsystem) + static_cast<unsigned>(seen_driver) +
            static_cast<unsigned>(seen_psi));
        result.confidence = 0.25F * result.corroborating_relations;
        return result;
    }

    result.domain = selected;
    result.state = DomainAssessmentState::Verified;
    result.corroborating_relations = 3U;
    result.confidence = 0.95F;
    return result;
}

} // namespace coreflow::semantic
