#include "coreflow/semantic/domain_evidence_aggregator.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

using namespace coreflow::semantic;

static Identifier128 make_id(std::uint8_t seed) {
    Identifier128 id{};
    id.bytes[0] = seed;
    return id;
}

int main() {
    DomainEvidenceAggregator aggregator{};
    const std::array<DomainRelationshipEvidence, 3> cpu{{
        {DomainEvidenceKind::SubsystemAssociation, FunctionalDomain::Cpu,
         EvidenceId{make_id(1)}, ProvenanceId{make_id(11)}, make_id(21), 9, 100, true},
        {DomainEvidenceKind::DriverAbi, FunctionalDomain::Cpu,
         EvidenceId{make_id(2)}, ProvenanceId{make_id(12)}, make_id(21), 9, 101, true},
        {DomainEvidenceKind::PsiResponseCorrelation, FunctionalDomain::Cpu,
         EvidenceId{make_id(3)}, ProvenanceId{make_id(13)}, make_id(21), 9, 102, true}
    }};
    auto verified = aggregator.aggregate(cpu, 9, 110, 1000);
    assert(verified.mutationEligible());
    assert(verified.domain == FunctionalDomain::Cpu);

    auto incomplete = aggregator.aggregate(
        std::span<const DomainRelationshipEvidence>(cpu.data(), 2), 9, 110, 1000);
    assert(!incomplete.mutationEligible());
    assert(incomplete.state == DomainAssessmentState::Candidate);
    assert(incomplete.domain == FunctionalDomain::Unresolved);

    auto stale = aggregator.aggregate(cpu, 10, 110, 1000);
    assert(!stale.mutationEligible());
    auto contradictory = cpu;
    contradictory[2].domain = FunctionalDomain::Gpu;
    auto conflict = aggregator.aggregate(contradictory, 9, 110, 1000);
    assert(!conflict.mutationEligible());
    assert(conflict.state == DomainAssessmentState::Unresolved);

    auto negative = cpu;
    negative[1].positive = false;
    assert(!aggregator.aggregate(negative, 9, 110, 1000).mutationEligible());

    std::cout << "P3 functional domain evidence aggregation: PASS\n";
    return 0;
}
