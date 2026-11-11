#include "coreflow/semantic/capability_resolver.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

using namespace coreflow::semantic;

namespace {
Identifier128 id(std::uint8_t seed) {
    Identifier128 value{};
    value.bytes[0] = seed;
    return value;
}

class FakeAdapter final : public ::coreflow::platform::DiscoveryAdapter {
public:
    std::vector<::coreflow::platform::RawDiscoveredEndpoint> endpoints{};
    std::vector<std::string> reads{};
    std::size_t read_index{0};
    coreflow::platform::NumericFormatStream format{true, true, true, 0, 100, 1};
    std::array<DomainRelationshipEvidence, 3> relationships{};
    std::size_t relationship_count{0};

    std::vector<::coreflow::platform::RawDiscoveredEndpoint> discover() noexcept override {
        return endpoints;
    }
    std::optional<std::string> readPassive(
        const ::coreflow::platform::RawDiscoveredEndpoint&) noexcept override {
        if (read_index >= reads.size()) return std::nullopt;
        return reads[read_index++];
    }
    coreflow::platform::NumericFormatStream readNumericFormat(
        const ::coreflow::platform::RawDiscoveredEndpoint&) noexcept override {
        return format;
    }
    std::size_t collectDomainRelationshipEvidence(
        const ::coreflow::platform::RawDiscoveredEndpoint&,
        std::span<DomainRelationshipEvidence> output) noexcept override {
        const auto copied = relationship_count < output.size() ? relationship_count : output.size();
        for (std::size_t i = 0; i < copied; ++i) output[i] = relationships[i];
        return relationship_count;
    }
};

::coreflow::platform::RawDiscoveredEndpoint endpoint(std::uint8_t seed, std::uint64_t generation = 7) {
    return {id(seed), "opaque-token-" + std::to_string(seed),
            AccessCapability::Read | AccessCapability::Write, generation};
}

std::array<DomainRelationshipEvidence, 3> domainEvidence(
    const ::coreflow::platform::RawDiscoveredEndpoint& node,
    FunctionalDomain domain = FunctionalDomain::Cpu,
    std::uint64_t generation = 7) {
    const CapabilityId capability{node.probe_id};
    return {{
        {DomainEvidenceKind::SubsystemAssociation, domain,
         EvidenceId{id(61)}, ProvenanceId{id(71)}, id(81), generation, 197, true, capability},
        {DomainEvidenceKind::DriverAbi, domain,
         EvidenceId{id(62)}, ProvenanceId{id(72)}, id(81), generation, 198, true, capability},
        {DomainEvidenceKind::PsiResponseCorrelation, domain,
         EvidenceId{id(63)}, ProvenanceId{id(73)}, id(81), generation, 199, true, capability}
    }};
}
}

int main() {
    CapabilityResolver resolver{};
    FakeAdapter adapter{};
    auto raw = endpoint(21);
    adapter.reads = {"42", "42"};

    // A stable value fingerprint alone is not a functional-domain proof.
    auto candidate = resolver.resolveOne(raw, adapter, 7, 200);
    assert(candidate.has_value());
    assert(candidate->state() == DescriptorState::Candidate);
    assert(candidate->value().kind == ValueKind::UnsignedInteger);
    assert(candidate->evidence().size() == 1);
    assert(candidate->evidence()[0].strength == EvidenceStrength::Weak);
    assert(candidate->semantic().functional_domain == FunctionalDomain::Unresolved);
    assert(!candidate->semantic().domain_verified);
    assert(candidate->mutation().mode == MutationMode::None);
    assert(!candidate->isMutationCapable(7, 201));
    assert(!resolver.resolveLocator(candidate->capabilityId(), 7).has_value());

    // Evidence sampled by the adapter after the resolver's caller captured now_ns
    // is accepted only within the bounded five-second monotonic observation window.
    const auto in_cycle_node = endpoint(40);
    FakeAdapter in_cycle_adapter{};
    in_cycle_adapter.reads = {"42", "42"};
    in_cycle_adapter.relationships = domainEvidence(in_cycle_node);
    in_cycle_adapter.relationships[2].observed_at_ns = 201;
    in_cycle_adapter.relationship_count = 3;
    const auto in_cycle = resolver.resolveOne(in_cycle_node, in_cycle_adapter, 7, 200);
    assert(in_cycle.has_value());
    assert(in_cycle->state() == DescriptorState::Validated);
    assert(in_cycle->domainAssessment().assessed_at_ns == 201);

    // Three independent, fresh, node-scoped relations promote the binding.
    const auto validated_node = endpoint(41);
    FakeAdapter complete_adapter{};
    complete_adapter.reads = {"42", "42"};
    complete_adapter.relationships = domainEvidence(validated_node);
    complete_adapter.relationship_count = 3;
    auto validated = resolver.resolveOne(validated_node, complete_adapter, 7, 200);
    assert(validated.has_value());
    assert(validated->state() == DescriptorState::Validated);
    assert(validated->semantic().functional_domain == FunctionalDomain::Cpu);
    assert(validated->semantic().domain_verified);
    assert(validated->semantic().domain.isValid());
    assert(validated->domainAssessment().domain == FunctionalDomain::Cpu);
    assert(validated->domainAssessment().corroborating_relations == 3);
    assert(validated->domainAssessment().confidence >= 0.95F);
    assert(validated->domainAssessment().generation == 7);
    assert(validated->domainAssessment().assessed_at_ns == 200);
    assert(validated->evidence().size() == 1);
    assert(validated->evidence()[0].strength == EvidenceStrength::Corroborated);
    assert(validated->mutation().mode == MutationMode::BoundedValue);
    assert(validated->isMutationCapable(7, 201));
    assert(resolver.resolveLocator(validated->capabilityId(), 7).has_value());
    assert(!resolver.resolveLocator(validated->capabilityId(), 8).has_value());

    // A newer failed cycle demotes the binding and revokes its old locator.
    auto volatile_node = validated_node;
    volatile_node.observed_generation = 8;
    FakeAdapter volatile_adapter{};
    auto demoted = resolver.resolveOne(volatile_node, volatile_adapter, 8, 300);
    assert(demoted.has_value());
    assert(demoted->state() == DescriptorState::Candidate);
    assert(demoted->mutation().mode == MutationMode::None);
    assert(!demoted->isMutationCapable());
    assert(!resolver.resolveLocator(validated->capabilityId(), 7).has_value());
    assert(!resolver.resolveLocator(validated->capabilityId(), 8).has_value());

    // Partial evidence is never sufficient to promote a semantic binding.
    const auto partial_node = endpoint(42);
    FakeAdapter partial_adapter{};
    partial_adapter.reads = {"42", "42"};
    partial_adapter.relationships = domainEvidence(partial_node);
    partial_adapter.relationship_count = 2;
    auto partial = resolver.resolveOne(partial_node, partial_adapter, 7, 200);
    assert(partial.has_value());
    assert(partial->state() == DescriptorState::Candidate);
    assert(partial->semantic().functional_domain == FunctionalDomain::Unresolved);
    assert(!partial->semantic().domain_verified);
    assert(partial->mutation().mode == MutationMode::None);
    assert(!partial->isMutationCapable());

    // A cross-domain contradiction leaves the aggregator unresolved.
    const auto conflict_node = endpoint(43);
    FakeAdapter conflict_adapter{};
    conflict_adapter.reads = {"42", "42"};
    conflict_adapter.relationships = domainEvidence(conflict_node);
    conflict_adapter.relationships[2].domain = FunctionalDomain::Gpu;
    conflict_adapter.relationship_count = 3;
    auto conflict = resolver.resolveOne(conflict_node, conflict_adapter, 7, 200);
    assert(conflict.has_value());
    assert(conflict->state() == DescriptorState::Candidate);
    assert(conflict->semantic().functional_domain == FunctionalDomain::Unresolved);
    assert(conflict->mutation().mode == MutationMode::None);
    assert(!conflict->isMutationCapable());
    assert(!resolver.resolveLocator(conflict->capabilityId(), 7).has_value());

    // Evidence belonging to another capability node must not be reused.
    const auto mismatch_node = endpoint(44);
    FakeAdapter mismatch_adapter{};
    mismatch_adapter.reads = {"42", "42"};
    mismatch_adapter.relationships = domainEvidence(mismatch_node);
    mismatch_adapter.relationships[1].capability = CapabilityId{id(99)};
    mismatch_adapter.relationship_count = 3;
    auto mismatch = resolver.resolveOne(mismatch_node, mismatch_adapter, 7, 200);
    assert(mismatch.has_value());
    assert(mismatch->state() == DescriptorState::Candidate);
    assert(mismatch->semantic().functional_domain == FunctionalDomain::Unresolved);
    assert(mismatch->mutation().mode == MutationMode::None);

    // Stale generations and mismatched relationship IDs fail closed.
    const auto stale_node = endpoint(45);
    FakeAdapter stale_evidence_adapter{};
    stale_evidence_adapter.reads = {"42", "42"};
    stale_evidence_adapter.relationships = domainEvidence(stale_node);
    stale_evidence_adapter.relationships[2].generation = 6;
    stale_evidence_adapter.relationship_count = 3;
    auto stale_evidence = resolver.resolveOne(stale_node, stale_evidence_adapter, 7, 200);
    assert(stale_evidence.has_value());
    assert(stale_evidence->state() == DescriptorState::Candidate);
    assert(stale_evidence->semantic().functional_domain == FunctionalDomain::Unresolved);

    const auto relation_mismatch_node = endpoint(46);
    FakeAdapter relation_mismatch_adapter{};
    relation_mismatch_adapter.reads = {"42", "42"};
    relation_mismatch_adapter.relationships = domainEvidence(relation_mismatch_node);
    relation_mismatch_adapter.relationships[1].relationship = id(82);
    relation_mismatch_adapter.relationship_count = 3;
    auto relation_mismatch = resolver.resolveOne(
        relation_mismatch_node, relation_mismatch_adapter, 7, 200);
    assert(relation_mismatch.has_value());
    assert(relation_mismatch->state() == DescriptorState::Candidate);
    assert(relation_mismatch->semantic().functional_domain == FunctionalDomain::Unresolved);

    FakeAdapter unstable_adapter{};
    unstable_adapter.reads = {"42", "43"};
    auto unstable = resolver.resolveOne(endpoint(22), unstable_adapter, 7, 200);
    assert(unstable.has_value());
    assert(unstable->state() == DescriptorState::Candidate);
    assert(unstable->value().kind == ValueKind::Unknown);
    assert(unstable->mutation().mode == MutationMode::None);
    assert(!unstable->isMutationCapable());

    FakeAdapter ambiguous_adapter{};
    ambiguous_adapter.reads = {"not a value!", "not a value!"};
    auto ambiguous = resolver.resolveOne(endpoint(23), ambiguous_adapter, 7, 200);
    assert(ambiguous.has_value());
    assert(ambiguous->state() == DescriptorState::Candidate);
    assert(ambiguous->value().kind == ValueKind::Unknown);
    assert(ambiguous->mutation().mode == MutationMode::None);

    FakeAdapter no_bounds_adapter{};
    no_bounds_adapter.reads = {"128", "128"};
    no_bounds_adapter.format = {};
    auto no_bounds = resolver.resolveOne(endpoint(24), no_bounds_adapter, 7, 200);
    assert(no_bounds.has_value());
    assert(no_bounds->state() == DescriptorState::Candidate);
    assert(no_bounds->mutation().mode == MutationMode::None);
    assert(!no_bounds->isMutationCapable());

    FakeAdapter boolean_adapter{};
    boolean_adapter.reads = {"1", "1"};
    boolean_adapter.format = {};
    auto boolean_value = resolver.resolveOne(endpoint(25), boolean_adapter, 7, 200);
    assert(boolean_value.has_value());
    assert(boolean_value->state() == DescriptorState::Candidate);
    assert(boolean_value->value().kind == ValueKind::Boolean);
    assert(boolean_value->mutation().mode == MutationMode::None);

    FakeAdapter stale_adapter{};
    stale_adapter.reads = {"1", "1"};
    assert(!resolver.resolveOne(endpoint(26, 6), stale_adapter, 7, 200).has_value());

    FakeAdapter batch_adapter{};
    batch_adapter.endpoints = {endpoint(31), endpoint(32)};
    batch_adapter.reads = {"0", "0", "schedutil", "schedutil"};
    const auto batch = resolver.resolveAll(batch_adapter, 7, 300);
    assert(batch.size() == 2);
    for (const auto& descriptor : batch) {
        assert(descriptor.state() == DescriptorState::Candidate);
        assert(descriptor.semantic().functional_domain == FunctionalDomain::Unresolved);
        assert(!descriptor.semantic().domain_verified);
        assert(descriptor.mutation().mode == MutationMode::None);
        assert(!descriptor.isMutationCapable());
    }
    assert(batch[1].value().kind == ValueKind::Enumeration);

    std::cout << "P3 capability resolver evidence-gated promotion: PASS\n";
    return 0;
}
