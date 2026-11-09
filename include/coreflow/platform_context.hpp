/**
 * @file platform_context.hpp
 * @brief Evidence-based Android platform identity and execution-tier selection.
 * @license MIT
 */
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace coreflow {

enum class PlatformVendor : uint8_t {
    Unknown, Xiaomi, Samsung, Google, Vivo, Oppo, OnePlus, Motorola, Sony, Asus, Other
};

enum class OsFamily : uint8_t {
    Unknown, AospLike, HyperOS, MIUI, OneUI, PixelOS, OriginOS, ColorOS, OxygenOS, FuntouchOS
};

enum class ExecutionTier : uint8_t {
    UniversalFallback = 0,
    XiaomiHyperOS = 1
};

enum class PolicyFamily : uint8_t {
    CoreFlowBase = 0,
    HyperOS4Inspired = 1
};

enum class IdentityConfidence : uint8_t {
    None = 0,
    Low = 1,
    Corroborated = 2,
    High = 3
};

enum PlatformEvidence : uint32_t {
    EvidenceNone = 0,
    EvidenceManufacturer = 1U << 0U,
    EvidenceBrand = 1U << 1U,
    EvidenceVendorManufacturer = 1U << 2U,
    EvidenceHyperOsVersion = 1U << 3U,
    EvidenceMiuiVersion = 1U << 4U,
    EvidenceBuildFingerprint = 1U << 5U,
    EvidenceAndroidRelease = 1U << 6U
};

// Abstract property source keeps detection hermetic and host-testable.
class AndroidPropertySource {
public:
    virtual ~AndroidPropertySource() = default;
    [[nodiscard]] virtual bool read(std::string_view key, std::string& value) const noexcept = 0;
};

class NativeAndroidPropertySource final : public AndroidPropertySource {
public:
    [[nodiscard]] bool read(std::string_view key, std::string& value) const noexcept override;
};

struct PlatformIdentity final {
    PlatformVendor vendor{PlatformVendor::Unknown};
    OsFamily os_family{OsFamily::Unknown};
    IdentityConfidence confidence{IdentityConfidence::None};
    uint32_t evidence_mask{EvidenceNone};
    uint32_t os_major{0};
    std::string manufacturer;
    std::string brand;
    std::string vendor_manufacturer;
    std::string product;
    std::string os_version;
    std::string android_release;
    std::string build_fingerprint;
};

struct PlatformContextSnapshot final {
    uint64_t generation{0};
    PlatformIdentity identity{};
    ExecutionTier execution_tier{ExecutionTier::UniversalFallback};
    PolicyFamily policy_family{PolicyFamily::CoreFlowBase};
    bool tier1_eligible{false};
    std::string tier_selection_reason{"platform identity not established"};

    [[nodiscard]] bool xiaomi_policy_enabled() const noexcept {
        return tier1_eligible &&
               execution_tier == ExecutionTier::XiaomiHyperOS &&
               policy_family == PolicyFamily::HyperOS4Inspired;
    }
};

class PlatformContextDetector final {
public:
    explicit PlatformContextDetector(const AndroidPropertySource& properties) noexcept
        : properties_(properties) {}

    [[nodiscard]] PlatformContextSnapshot detect(uint64_t generation = 0) const noexcept;

private:
    const AndroidPropertySource& properties_;
};

} // namespace coreflow
