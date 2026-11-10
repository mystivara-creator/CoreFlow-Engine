#include "coreflow/semantic/descriptor_factory.hpp"
#include "coreflow/semantic/endpoint_locator.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>

using namespace coreflow::semantic;

namespace {
Identifier128 id(std::uint8_t seed) {
    Identifier128 result{};
    result.bytes[0] = seed;
    return result;
}

template <typename T>
TypedId<T> typed(std::uint8_t seed) { return TypedId<T>{id(seed)}; }

DescriptorDraft draft(std::uint8_t cap, EvidenceStrength strength = EvidenceStrength::Strong,
                      std::uint64_t generation = 7) {
    DescriptorDraft d{};
    d.capability = typed<CapabilityTag>(cap);
    d.semantic = {typed<SemanticRoleTag>(10), typed<SemanticDomainTag>(11), typed<VocabularyTag>(12), 0.95F};
    d.value.kind = ValueKind::UnsignedInteger;
    d.value.numeric = {true, true, true, 0, 100, 1};
    d.value.supports_readback = true;
    d.mutation = {MutationMode::BoundedValue, true, true, true};
    d.access = AccessCapability::Read | AccessCapability::Write;
    d.risk = {typed<RiskClassTag>(13), 0.1F, 0.9F};
    d.provenance = {typed<ProvenanceTag>(14), id(15), generation, 100, 10000};
    d.evidence.push_back({typed<EvidenceTag>(16), d.provenance.id, typed<VocabularyTag>(17), strength,
                          typed<ObservationTag>(18), generation, 101});
    d.generation = generation;
    return d;
}

int strategy_decision(const DynamicSemanticDescriptor& d) {
    // Strategy projection excludes endpoint and capability identity.
    const auto role = d.semantic().role.value.bytes[0];
    const auto domain = d.semantic().domain.value.bytes[0];
    return static_cast<int>(role) * 257 + static_cast<int>(domain) * 17 +
           static_cast<int>(d.value().kind) * 3 + static_cast<int>(d.mutation().mode);
}
}

int main() {
    CapabilityResolver resolver{};

    auto first = resolver.validateAndIssue(draft(21), 7, 200);
    auto second = resolver.validateAndIssue(draft(22), 7, 200);
    assert(first && second && first->isMutationCapable() && second->isMutationCapable());
    assert(strategy_decision(*first) == strategy_decision(*second));

    auto unsupported = resolver.validateAndIssue(draft(23, EvidenceStrength::None), 7, 200);
    assert(unsupported.has_value());
    assert(!unsupported->isMutationCapable());

    auto stale = resolver.validateAndIssue(draft(24, EvidenceStrength::Strong, 6), 7, 200);
    assert(!stale);

    platform::EndpointRegistry registry{};
    platform::EndpointLocator locator{id(31), platform::LocatorKind::RuntimeHandle, "opaque-adapter-token", 7};
    assert(registry.bind(typed<CapabilityTag>(21), locator));
    assert(registry.resolve(typed<CapabilityTag>(21), 7).has_value());
    assert(!registry.resolve(typed<CapabilityTag>(21), 8).has_value());

    std::cout << "P3-A1 semantic foundation: PASS\n";
    return 0;
}
