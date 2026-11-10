#pragma once

#include <array>
#include <cstdint>

namespace coreflow::semantic {

struct Identifier128 final {
    std::array<std::uint8_t, 16> bytes{};
    [[nodiscard]] constexpr bool isValid() const noexcept {
        for (const auto byte : bytes) if (byte != 0U) return true;
        return false;
    }
    friend constexpr bool operator==(const Identifier128&, const Identifier128&) noexcept = default;
};

template <typename Tag>
struct TypedId final {
    Identifier128 value{};
    [[nodiscard]] constexpr bool isValid() const noexcept { return value.isValid(); }
    friend constexpr bool operator==(const TypedId&, const TypedId&) noexcept = default;
};

struct CapabilityTag;
struct SemanticRoleTag;
struct SemanticDomainTag;
struct EvidenceTag;
struct ProvenanceTag;
struct VocabularyTag;
struct ObservationTag;
struct UnitTag;
struct RiskClassTag;
struct DependencyTag;

using CapabilityId = TypedId<CapabilityTag>;
using SemanticRoleId = TypedId<SemanticRoleTag>;
using SemanticDomainId = TypedId<SemanticDomainTag>;
using EvidenceId = TypedId<EvidenceTag>;
using ProvenanceId = TypedId<ProvenanceTag>;
using VocabularyId = TypedId<VocabularyTag>;
using ObservationId = TypedId<ObservationTag>;
using UnitId = TypedId<UnitTag>;
using RiskClassId = TypedId<RiskClassTag>;
using DependencyId = TypedId<DependencyTag>;

enum class ValueKind : std::uint8_t { Unknown, SignedInteger, UnsignedInteger, FloatingPoint, Boolean, Enumeration, Opaque };
enum class AccessCapability : std::uint8_t { None = 0, Read = 1, Write = 2, Subscribe = 4 };
enum class MutationMode : std::uint8_t { None, DirectValue, BoundedValue, EnumeratedValue, Transactional };
enum class EvidenceStrength : std::uint8_t { None, Weak, Corroborated, Strong, Verified };
enum class DescriptorState : std::uint8_t { Candidate, Resolved, Validated, Stale, Revoked };

[[nodiscard]] constexpr AccessCapability operator|(AccessCapability lhs, AccessCapability rhs) noexcept {
    return static_cast<AccessCapability>(static_cast<std::uint8_t>(lhs) | static_cast<std::uint8_t>(rhs));
}
[[nodiscard]] constexpr bool hasAccess(AccessCapability set, AccessCapability requested) noexcept {
    return (static_cast<std::uint8_t>(set) & static_cast<std::uint8_t>(requested)) ==
           static_cast<std::uint8_t>(requested);
}

} // namespace coreflow::semantic
