#include "coreflow/semantic/capability_resolver.hpp"

#include <array>
#include <charconv>
#include <cmath>
#include <limits>
#include <string_view>
#include <system_error>
#include <utility>

namespace coreflow::semantic {
namespace {

struct ParsedValue final {
    ValueKind kind{ValueKind::Unknown};
    bool valid{false};
    bool enumeration{false};
};

[[nodiscard]] bool isToken(std::string_view value) noexcept {
    if (value.empty()) return false;
    for (const unsigned char c : value) {
        const bool allowed = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                             (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
        if (!allowed) return false;
    }
    return true;
}

[[nodiscard]] ParsedValue parseValue(std::string_view value) noexcept {
    if (value == "0" || value == "1") return {ValueKind::Boolean, true, true};
    if (value.empty()) return {};

    // Non-negative integral streams are classified as unsigned; negative streams as signed.
    if (value.front() != '-') {
        std::uint64_t unsigned_value = 0;
        const auto unsigned_result = std::from_chars(value.data(), value.data() + value.size(), unsigned_value);
        if (unsigned_result.ec == std::errc{} && unsigned_result.ptr == value.data() + value.size())
            return {ValueKind::UnsignedInteger, true, false};
    }

    std::int64_t signed_value = 0;
    const auto signed_result = std::from_chars(value.data(), value.data() + value.size(), signed_value);
    if (signed_result.ec == std::errc{} && signed_result.ptr == value.data() + value.size())
        return {ValueKind::SignedInteger, true, false};

    double floating_value = 0.0;
    const auto float_result = std::from_chars(value.data(), value.data() + value.size(),
                                               floating_value, std::chars_format::general);
    if (float_result.ec == std::errc{} && float_result.ptr == value.data() + value.size() &&
        std::isfinite(floating_value))
        return {ValueKind::FloatingPoint, true, false};

    if (isToken(value)) return {ValueKind::Enumeration, true, true};
    return {};
}

[[nodiscard]] Identifier128 makeId(std::uint64_t first, std::uint64_t second) noexcept {
    Identifier128 result{};
    for (std::size_t i = 0; i < 8; ++i) {
        result.bytes[i] = static_cast<std::uint8_t>((first >> (i * 8U)) & 0xffU);
        result.bytes[i + 8] = static_cast<std::uint8_t>((second >> (i * 8U)) & 0xffU);
    }
    if (!result.isValid()) result.bytes[0] = 1U;
    return result;
}

[[nodiscard]] std::uint64_t mix(std::uint64_t value) noexcept {
    value ^= value >> 30U;
    value *= 0xbf58476d1ce4e5b9ULL;
    value ^= value >> 27U;
    value *= 0x94d049bb133111ebULL;
    value ^= value >> 31U;
    return value;
}

[[nodiscard]] std::uint64_t fold(const Identifier128& id) noexcept {
    std::uint64_t value = 0xcbf29ce484222325ULL;
    for (const auto byte : id.bytes) {
        value ^= byte;
        value *= 0x100000001b3ULL;
    }
    return mix(value);
}

[[nodiscard]] std::uint64_t saturatingAdd(std::uint64_t lhs, std::uint64_t rhs) noexcept {
    return rhs > std::numeric_limits<std::uint64_t>::max() - lhs
               ? std::numeric_limits<std::uint64_t>::max() : lhs + rhs;
}

} // namespace

std::vector<DynamicSemanticDescriptor> CapabilityResolver::resolveAll(
    ::coreflow::platform::DiscoveryAdapter& adapter, std::uint64_t active_generation,
    std::uint64_t now_ns) noexcept {
    std::vector<DynamicSemanticDescriptor> descriptors{};
    const auto endpoints = adapter.discover();
    descriptors.reserve(endpoints.size());
    for (const auto& endpoint : endpoints) {
        auto descriptor = buildDescriptor(endpoint, adapter, active_generation, now_ns);
        if (descriptor.has_value()) descriptors.push_back(std::move(*descriptor));
    }
    return descriptors;
}

std::optional<DynamicSemanticDescriptor> CapabilityResolver::resolveOne(
    const ::coreflow::platform::RawDiscoveredEndpoint& endpoint, ::coreflow::platform::DiscoveryAdapter& adapter,
    std::uint64_t active_generation, std::uint64_t now_ns) noexcept {
    return buildDescriptor(endpoint, adapter, active_generation, now_ns);
}

std::optional<platform::EndpointLocator> CapabilityResolver::resolveLocator(
    CapabilityId capability, std::uint64_t active_generation) const {
    return endpoint_registry_.resolve(capability, active_generation);
}

std::optional<DynamicSemanticDescriptor> CapabilityResolver::buildDescriptor(
    const ::coreflow::platform::RawDiscoveredEndpoint& endpoint, ::coreflow::platform::DiscoveryAdapter& adapter,
    std::uint64_t active_generation, std::uint64_t now_ns) noexcept {
    if (!endpoint.probe_id.isValid() || endpoint.opaque_locator.empty() ||
        endpoint.observed_generation != active_generation) return std::nullopt;

    const auto first_read = adapter.readPassive(endpoint);
    const auto second_read = adapter.readPassive(endpoint);
    const bool stable = first_read.has_value() && second_read.has_value() &&
                        *first_read == *second_read;

    ParsedValue parsed{};
    if (stable) parsed = parseValue(*first_read);

    const auto raw_format = adapter.readNumericFormat(endpoint);
    NumericConstraints constraints{raw_format.has_minimum, raw_format.has_maximum,
                                   raw_format.has_resolution, raw_format.minimum,
                                   raw_format.maximum, raw_format.resolution};
    const bool format_valid = constraints.isValid();
    if (!format_valid) constraints = {};

    const bool type_verified = stable && parsed.valid && parsed.kind != ValueKind::Unknown;
    const bool corroborated = type_verified && format_valid;
    const DescriptorState state = corroborated ? DescriptorState::Validated : DescriptorState::Candidate;

    ValueModel value{};
    value.kind = type_verified ? parsed.kind : ValueKind::Unknown;
    value.numeric = constraints;
    value.supports_readback = first_read.has_value() && second_read.has_value();
    value.supports_enumeration = type_verified && parsed.enumeration;

    MutationSemantics mutation{};
    mutation.mode = MutationMode::None;
    if (state == DescriptorState::Validated &&
        hasAccess(endpoint.raw_access, AccessCapability::Write)) {
        if (parsed.kind == ValueKind::Boolean || parsed.kind == ValueKind::Enumeration) {
            mutation.mode = MutationMode::EnumeratedValue;
            mutation.reversible = true;
        } else if ((parsed.kind == ValueKind::SignedInteger ||
                    parsed.kind == ValueKind::UnsignedInteger ||
                    parsed.kind == ValueKind::FloatingPoint) &&
                   constraints.has_minimum && constraints.has_maximum) {
            mutation.mode = MutationMode::BoundedValue;
            mutation.reversible = true;
        }
    }

    // Semantic identity is derived only from observed behavior and format metadata.
    // Endpoint identity and opaque locator are deliberately excluded.
    const std::uint64_t fingerprint =
        (static_cast<std::uint64_t>(value.kind) << 8U) ^
        (static_cast<std::uint64_t>(endpoint.raw_access) << 16U) ^
        (constraints.has_minimum ? 0x101ULL : 0ULL) ^
        (constraints.has_maximum ? 0x202ULL : 0ULL) ^
        (constraints.has_resolution ? 0x404ULL : 0ULL) ^
        mix(static_cast<std::uint64_t>(constraints.minimum)) ^
        mix(static_cast<std::uint64_t>(constraints.maximum) + 0x10001ULL) ^
        mix(static_cast<std::uint64_t>(constraints.resolution) + 0x20003ULL) ^
        (value.supports_enumeration ? 0x808ULL : 0ULL);
    const auto role_id = makeId(mix(fingerprint ^ 0x524f4c45ULL), mix(fingerprint));
    const auto domain_id = makeId(mix(fingerprint ^ 0x444f4d41494eULL), mix(fingerprint + 1U));
    const auto vocabulary_id = makeId(mix(static_cast<std::uint64_t>(value.kind) + 0x564f434142ULL),
                                      mix(fingerprint + 2U));

    SemanticBinding semantic{
        SemanticRoleId{role_id}, SemanticDomainId{domain_id}, VocabularyId{vocabulary_id},
        corroborated ? 0.90F : (type_verified ? 0.45F : 0.10F)};

    const auto provider = makeId(fold(endpoint.probe_id), mix(endpoint.observed_generation));
    const auto provenance_id = ProvenanceId{makeId(mix(fold(endpoint.probe_id) ^ now_ns),
                                                    mix(active_generation ^ 0x50524f56ULL))};
    CapabilityProvenance provenance{provenance_id, provider, active_generation, now_ns,
                                    saturatingAdd(now_ns, 60'000'000'000ULL)};

    std::vector<CapabilityEvidence> evidence{};
    evidence.reserve(1);
    const auto evidence_strength = corroborated ? EvidenceStrength::Corroborated
                                                : (type_verified ? EvidenceStrength::Weak
                                                                 : EvidenceStrength::None);
    evidence.push_back(CapabilityEvidence{
        EvidenceId{makeId(mix(fold(endpoint.probe_id) ^ now_ns), mix(active_generation + 0x45564944ULL))},
        provenance_id, vocabulary_id, evidence_strength,
        ObservationId{makeId(mix(fold(endpoint.probe_id) + 1U), mix(now_ns + 0x4f4253ULL))},
        active_generation, now_ns});

    const CapabilityRisk risk{RiskClassId{makeId(0x5249534b434c4153ULL, fingerprint)},
                              mutation.mode == MutationMode::None ? 0.75F : 0.25F,
                              corroborated ? 0.85F : 0.20F};

    DynamicSemanticDescriptor descriptor{
        CapabilityId{endpoint.probe_id}, semantic, value, mutation, endpoint.raw_access, risk,
        provenance, std::move(evidence), active_generation, state};

    if (state == DescriptorState::Validated) {
        platform::EndpointLocator locator{makeId(fold(endpoint.probe_id), 0x4c4f4341544f52ULL),
                                           platform::LocatorKind::RuntimeHandle,
                                           endpoint.opaque_locator, active_generation};
        if (!endpoint_registry_.bind(descriptor.capabilityId(), std::move(locator)))
            return std::nullopt;
    }
    return descriptor;
}

} // namespace coreflow::semantic
