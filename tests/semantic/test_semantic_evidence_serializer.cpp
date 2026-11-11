#include "coreflow/semantic/capability_resolver.hpp"
#include "coreflow/semantic/semantic_evidence_serializer.hpp"

#include <algorithm>
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

Identifier128 makeId(std::uint8_t seed) {
    Identifier128 value{};
    value.bytes[0] = seed;
    return value;
}

class FixtureAdapter final : public ::coreflow::platform::DiscoveryAdapter {
public:
    std::array<DomainRelationshipEvidence, 3> relationships{};
    std::size_t relationship_count{0};

    std::vector<::coreflow::platform::RawDiscoveredEndpoint> discover() noexcept override {
        return {};
    }

    std::optional<std::string> readPassive(
        const ::coreflow::platform::RawDiscoveredEndpoint&) noexcept override {
        return std::string("600000");
    }

    ::coreflow::platform::NumericFormatStream readNumericFormat(
        const ::coreflow::platform::RawDiscoveredEndpoint&) noexcept override {
        return {true, true, false, 300000, 1200000, 0};
    }

    std::size_t collectDomainRelationshipEvidence(
        const ::coreflow::platform::RawDiscoveredEndpoint&,
        std::span<DomainRelationshipEvidence> output) noexcept override {
        const auto copied = std::min(relationship_count, output.size());
        for (std::size_t i = 0; i < copied; ++i) output[i] = relationships[i];
        return relationship_count;
    }
};

::coreflow::platform::RawDiscoveredEndpoint makeEndpoint(std::uint8_t seed) {
    return {makeId(seed), "test:read-only-endpoint",
            AccessCapability::Read, 7U};
}

std::array<DomainRelationshipEvidence, 3> makeEvidence(
    const ::coreflow::platform::RawDiscoveredEndpoint& endpoint) {
    const CapabilityId capability{endpoint.probe_id};
    const auto relation = makeId(80);
    return {{
        {DomainEvidenceKind::SubsystemAssociation, FunctionalDomain::Cpu,
         EvidenceId{makeId(61)}, ProvenanceId{makeId(71)}, relation, 7U, 100U, true, capability},
        {DomainEvidenceKind::DriverAbi, FunctionalDomain::Cpu,
         EvidenceId{makeId(62)}, ProvenanceId{makeId(72)}, relation, 7U, 101U, true, capability},
        {DomainEvidenceKind::PsiResponseCorrelation, FunctionalDomain::Cpu,
         EvidenceId{makeId(63)}, ProvenanceId{makeId(73)}, relation, 7U, 102U, true, capability}
    }};
}

std::vector<DynamicSemanticDescriptor> resolve(
    FixtureAdapter& adapter,
    const ::coreflow::platform::RawDiscoveredEndpoint& endpoint,
    std::uint64_t now_ns) {
    CapabilityResolver resolver{};
    auto descriptor = resolver.resolveOne(endpoint, adapter, 7U, now_ns);
    assert(descriptor.has_value());
    std::vector<DynamicSemanticDescriptor> result{};
    result.push_back(std::move(*descriptor));
    return result;
}

} // namespace

int main() {
    const auto unavailable = serializeSemanticEvidence(
        std::span<const DynamicSemanticDescriptor>{}, 10'000U);
    assert(unavailable.find("\"schemaVersion\":1") != std::string::npos);
    assert(unavailable.find("\"status\":\"unavailable\"") != std::string::npos);
    assert(unavailable.find("\"bindings\":[]") != std::string::npos);

    const auto candidate_endpoint = makeEndpoint(21);
    FixtureAdapter candidate_adapter{};
    candidate_adapter.relationships = makeEvidence(candidate_endpoint);
    candidate_adapter.relationships[2].observed_at_ns = 199U;
    candidate_adapter.relationship_count = 2U;
    auto candidate = resolve(candidate_adapter, candidate_endpoint, 200U);
    assert(candidate.front().state() == DescriptorState::Candidate);
    assert(candidate.front().domainAssessment().state == DomainAssessmentState::Candidate);
    const auto candidate_json = serializeSemanticEvidence(candidate, 200U);
    assert(candidate_json.find("\"status\":\"available\"") != std::string::npos);
    assert(candidate_json.find("\"bindingState\":\"candidate\"") != std::string::npos);
    assert(candidate_json.find("\"domainAssessment\":{\"state\":\"candidate\"") != std::string::npos);
    assert(candidate_json.find("\"corroboratingRelations\":2") != std::string::npos);
    assert(candidate_json.find("\"freshness\":\"fresh\"") != std::string::npos);

    const auto validated_endpoint = makeEndpoint(22);
    FixtureAdapter validated_adapter{};
    validated_adapter.relationships = makeEvidence(validated_endpoint);
    validated_adapter.relationship_count = 3U;
    auto validated = resolve(validated_adapter, validated_endpoint, 200U);
    assert(validated.front().state() == DescriptorState::Validated);
    assert(validated.front().domainAssessment().state == DomainAssessmentState::Verified);
    assert(validated.front().domainAssessment().domain == FunctionalDomain::Cpu);

    const auto validated_json = serializeSemanticEvidence(validated, 200U);
    assert(validated_json.find("\"bindingState\":\"validated\"") != std::string::npos);
    assert(validated_json.find("\"domain\":\"cpu\"") != std::string::npos);
    assert(validated_json.find("\"state\":\"verified\"") != std::string::npos);
    assert(validated_json.find("\"corroboratingRelations\":3") != std::string::npos);
    assert(validated_json.find("\"generation\":\"7\"") != std::string::npos);
    assert(validated_json.find("\"observedAtMonotonicNs\":\"200\"") != std::string::npos);
    assert(validated_json.find("\"freshness\":\"fresh\"") != std::string::npos);

    const auto boundary_fresh = serializeSemanticEvidence(
        validated, 5'000'000'200ULL);
    assert(boundary_fresh.find("\"ageMs\":5000.000000") != std::string::npos);
    assert(boundary_fresh.find("\"freshness\":\"fresh\"") != std::string::npos);

    const auto stale = serializeSemanticEvidence(
        validated, 5'000'000'201ULL);
    assert(stale.find("\"ageMs\":5000.000001") != std::string::npos);
    assert(stale.find("\"freshness\":\"stale\"") != std::string::npos);

    std::cout << "P3 native semantic evidence serialization contract: PASS" << std::endl;
    return 0;
}
