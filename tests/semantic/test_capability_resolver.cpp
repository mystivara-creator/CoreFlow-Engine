#include "coreflow/semantic/capability_resolver.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
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

class FakeAdapter final : public platform::DiscoveryAdapter {
public:
    std::vector<platform::RawDiscoveredEndpoint> endpoints{};
    std::vector<std::string> reads{};
    std::size_t read_index{0};
    platform::NumericFormatStream format{true, true, true, 0, 100, 1};

    std::vector<platform::RawDiscoveredEndpoint> discover() noexcept override { return endpoints; }
    std::optional<std::string> readPassive(
        const platform::RawDiscoveredEndpoint&) noexcept override {
        if (read_index >= reads.size()) return std::nullopt;
        return reads[read_index++];
    }
    platform::NumericFormatStream readNumericFormat(
        const platform::RawDiscoveredEndpoint&) noexcept override { return format; }
};

platform::RawDiscoveredEndpoint endpoint(std::uint8_t seed, std::uint64_t generation = 7) {
    return {id(seed), "opaque-token-" + std::to_string(seed),
            AccessCapability::Read | AccessCapability::Write, generation};
}
}

int main() {
    CapabilityResolver resolver{};
    FakeAdapter adapter{};
    auto raw = endpoint(21);
    adapter.reads = {"42", "42"};

    auto validated = resolver.resolveOne(raw, adapter, 7, 200);
    assert(validated.has_value());
    assert(validated->state() == DescriptorState::Validated);
    assert(validated->value().kind == ValueKind::UnsignedInteger);
    assert(validated->evidence().size() == 1);
    assert(validated->evidence()[0].strength == EvidenceStrength::Corroborated);
    assert(validated->mutation().mode == MutationMode::BoundedValue);
    assert(validated->isMutationCapable(7, 201));
    assert(resolver.resolveLocator(validated->capabilityId(), 7).has_value());
    assert(!resolver.resolveLocator(validated->capabilityId(), 8).has_value());

    FakeAdapter unstable_adapter{};
    unstable_adapter.reads = {"42", "43"};
    auto unstable = resolver.resolveOne(endpoint(22), unstable_adapter, 7, 200);
    assert(unstable.has_value());
    assert(unstable->state() == DescriptorState::Candidate);
    assert(unstable->mutation().mode == MutationMode::None);
    assert(!unstable->isMutationCapable());
    assert(!resolver.resolveLocator(unstable->capabilityId(), 7).has_value());

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
    assert(no_bounds->state() == DescriptorState::Validated);
    assert(no_bounds->mutation().mode == MutationMode::None);
    assert(!no_bounds->isMutationCapable());

    FakeAdapter boolean_adapter{};
    boolean_adapter.reads = {"1", "1"};
    boolean_adapter.format = {};
    auto boolean_value = resolver.resolveOne(endpoint(25), boolean_adapter, 7, 200);
    assert(boolean_value.has_value());
    assert(boolean_value->state() == DescriptorState::Validated);
    assert(boolean_value->value().kind == ValueKind::Boolean);
    assert(boolean_value->mutation().mode == MutationMode::EnumeratedValue);

    FakeAdapter stale_adapter{};
    stale_adapter.reads = {"1", "1"};
    assert(!resolver.resolveOne(endpoint(26, 6), stale_adapter, 7, 200).has_value());

    FakeAdapter batch_adapter{};
    batch_adapter.endpoints = {endpoint(31), endpoint(32)};
    batch_adapter.reads = {"0", "0", "schedutil", "schedutil"};
    const auto batch = resolver.resolveAll(batch_adapter, 7, 300);
    assert(batch.size() == 2);
    assert(batch[0].state() == DescriptorState::Validated);
    assert(batch[1].state() == DescriptorState::Validated);
    assert(batch[1].value().kind == ValueKind::Enumeration);

    std::cout << "P3-A2 capability resolver: PASS\n";
    return 0;
}
