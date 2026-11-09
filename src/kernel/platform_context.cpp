/**
 * @file platform_context.cpp
 * @brief Conservative Android property corroboration and tier selection.
 * @license MIT
 */
#include "coreflow/platform_context.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <string>
#include <string_view>
#include <utility>

#if defined(__ANDROID__)
#include <sys/system_properties.h>
#endif

namespace coreflow {
namespace {

std::string lower_ascii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool read_nonempty(const AndroidPropertySource& source, std::string_view key,
                   std::string& output, uint32_t evidence, uint32_t& mask) noexcept {
    std::string value;
    if (!source.read(key, value) || value.empty()) return false;
    output = std::move(value);
    mask |= evidence;
    return true;
}

bool starts_with(std::string_view value, std::string_view prefix) noexcept {
    return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
}

bool is_xiaomi_family(std::string_view value) noexcept {
    return value == "xiaomi" || value == "redmi" || value == "poco";
}

PlatformVendor detect_vendor(std::string_view manufacturer, std::string_view brand,
                             std::string_view vendor_manufacturer,
                             IdentityConfidence& confidence) noexcept {
    const auto m = lower_ascii(std::string(manufacturer));
    const auto b = lower_ascii(std::string(brand));
    const auto vm = lower_ascii(std::string(vendor_manufacturer));

    const bool xiaomi_manufacturer = m == "xiaomi" || vm == "xiaomi";
    const bool xiaomi_brand = is_xiaomi_family(b);
    if (xiaomi_manufacturer || xiaomi_brand) {
        const bool conflicting_manufacturer =
            (!m.empty() && m != "xiaomi") || (!vm.empty() && vm != "xiaomi");
        const bool conflicting_brand = !b.empty() && !xiaomi_brand;
        if ((xiaomi_manufacturer && conflicting_brand) ||
            (xiaomi_brand && conflicting_manufacturer)) {
            confidence = IdentityConfidence::Low;
            return PlatformVendor::Unknown;
        }
        confidence = xiaomi_manufacturer && xiaomi_brand
            ? IdentityConfidence::High : IdentityConfidence::Corroborated;
        return PlatformVendor::Xiaomi;
    }
    if (m == "samsung" || b == "samsung") {
        confidence = (m == "samsung" && b == "samsung")
            ? IdentityConfidence::High : IdentityConfidence::Corroborated;
        return PlatformVendor::Samsung;
    }
    if (m == "google" || b == "google") {
        confidence = (m == "google" && b == "google")
            ? IdentityConfidence::High : IdentityConfidence::Corroborated;
        return PlatformVendor::Google;
    }
    if (m == "vivo" || b == "vivo") { confidence = IdentityConfidence::Corroborated; return PlatformVendor::Vivo; }
    if (m == "oppo" || b == "oppo") { confidence = IdentityConfidence::Corroborated; return PlatformVendor::Oppo; }
    if (m == "oneplus" || b == "oneplus") { confidence = IdentityConfidence::Corroborated; return PlatformVendor::OnePlus; }
    if (m == "motorola" || b == "motorola") { confidence = IdentityConfidence::Corroborated; return PlatformVendor::Motorola; }
    if (m == "sony" || b == "sony") { confidence = IdentityConfidence::Corroborated; return PlatformVendor::Sony; }
    if (m == "asus" || b == "asus") { confidence = IdentityConfidence::Corroborated; return PlatformVendor::Asus; }
    if (!m.empty() || !b.empty() || !vm.empty()) {
        confidence = IdentityConfidence::Low;
        return PlatformVendor::Other;
    }
    confidence = IdentityConfidence::None;
    return PlatformVendor::Unknown;
}

uint32_t parse_major(std::string_view value) noexcept {
    const size_t first = value.find_first_of("0123456789");
    if (first == std::string_view::npos) return 0;
    value.remove_prefix(first);
    uint32_t major = 0;
    const auto result = std::from_chars(value.data(), value.data() + value.size(), major, 10);
    return result.ec == std::errc{} ? major : 0;
}

} // namespace

std::string_view platform_vendor_name(PlatformVendor value) noexcept {
    switch (value) {
        case PlatformVendor::Xiaomi: return "xiaomi";
        case PlatformVendor::Samsung: return "samsung";
        case PlatformVendor::Google: return "google";
        case PlatformVendor::Vivo: return "vivo";
        case PlatformVendor::Oppo: return "oppo";
        case PlatformVendor::OnePlus: return "oneplus";
        case PlatformVendor::Motorola: return "motorola";
        case PlatformVendor::Sony: return "sony";
        case PlatformVendor::Asus: return "asus";
        case PlatformVendor::Other: return "other";
        case PlatformVendor::Unknown: return "unknown";
    }
    return "unknown";
}

std::string_view os_family_name(OsFamily value) noexcept {
    switch (value) {
        case OsFamily::AospLike: return "aosp_like";
        case OsFamily::HyperOS: return "hyperos";
        case OsFamily::MIUI: return "miui";
        case OsFamily::OneUI: return "oneui";
        case OsFamily::PixelOS: return "pixel_os";
        case OsFamily::OriginOS: return "originos";
        case OsFamily::ColorOS: return "coloros";
        case OsFamily::OxygenOS: return "oxygenos";
        case OsFamily::FuntouchOS: return "funtouchos";
        case OsFamily::Unknown: return "unknown";
    }
    return "unknown";
}

std::string_view execution_tier_name(ExecutionTier value) noexcept {
    return value == ExecutionTier::XiaomiHyperOS ? "xiaomi_hyperos" : "universal_fallback";
}

std::string_view policy_family_name(PolicyFamily value) noexcept {
    return value == PolicyFamily::HyperOS4Inspired ? "hyperos4_inspired" : "coreflow_base";
}

std::string_view identity_confidence_name(IdentityConfidence value) noexcept {
    switch (value) {
        case IdentityConfidence::None: return "none";
        case IdentityConfidence::Low: return "low";
        case IdentityConfidence::Corroborated: return "corroborated";
        case IdentityConfidence::High: return "high";
    }
    return "none";
}

bool NativeAndroidPropertySource::read(std::string_view key, std::string& value) const noexcept {
    value.clear();
#if defined(__ANDROID__)
    if (key.empty() || key.size() >= PROP_NAME_MAX) return false;
    std::array<char, PROP_NAME_MAX> key_buffer{};
    std::array<char, PROP_VALUE_MAX> value_buffer{};
    std::copy(key.begin(), key.end(), key_buffer.begin());
    const int length = __system_property_get(key_buffer.data(), value_buffer.data());
    if (length <= 0 || static_cast<size_t>(length) >= value_buffer.size()) return false;
    value.assign(value_buffer.data(), static_cast<size_t>(length));
    return !value.empty();
#else
    (void)key;
    return false;
#endif
}

PlatformContextSnapshot PlatformContextDetector::detect(uint64_t generation) const noexcept {
    PlatformContextSnapshot snapshot{};
    snapshot.generation = generation;
    auto& identity = snapshot.identity;

    read_nonempty(properties_, "ro.product.manufacturer", identity.manufacturer,
                  EvidenceManufacturer, identity.evidence_mask);
    read_nonempty(properties_, "ro.product.brand", identity.brand,
                  EvidenceBrand, identity.evidence_mask);
    read_nonempty(properties_, "ro.product.vendor.manufacturer", identity.vendor_manufacturer,
                  EvidenceVendorManufacturer, identity.evidence_mask);
    read_nonempty(properties_, "ro.product.name", identity.product,
                  EvidenceNone, identity.evidence_mask);
    read_nonempty(properties_, "ro.mi.os.version.name", identity.os_version,
                  EvidenceHyperOsVersion, identity.evidence_mask);
    read_nonempty(properties_, "ro.build.version.release", identity.android_release,
                  EvidenceAndroidRelease, identity.evidence_mask);
    read_nonempty(properties_, "ro.build.fingerprint", identity.build_fingerprint,
                  EvidenceBuildFingerprint, identity.evidence_mask);

    std::string miui_version;
    const bool has_miui = read_nonempty(properties_, "ro.miui.ui.version.name", miui_version,
                                        EvidenceMiuiVersion, identity.evidence_mask);
    identity.vendor = detect_vendor(identity.manufacturer, identity.brand,
                                     identity.vendor_manufacturer, identity.confidence);
    const auto fingerprint_lower = lower_ascii(identity.build_fingerprint);
    identity.fingerprint_vendor_match =
        starts_with(fingerprint_lower, "xiaomi/") ||
        starts_with(fingerprint_lower, "redmi/") ||
        starts_with(fingerprint_lower, "poco/");
    if (identity.vendor == PlatformVendor::Xiaomi && !identity.fingerprint_vendor_match &&
        identity.confidence == IdentityConfidence::High) {
        // Custom ROM or missing vendor fingerprint is not proof of a different
        // device, but it is insufficient evidence for Xiaomi-exclusive policy.
        identity.confidence = IdentityConfidence::Corroborated;
    }

    if (!identity.os_version.empty()) {
        identity.os_family = OsFamily::HyperOS;
        identity.os_major = parse_major(identity.os_version);
    } else if (has_miui) {
        identity.os_family = OsFamily::MIUI;
        identity.os_version = std::move(miui_version);
        identity.os_major = parse_major(identity.os_version);
    } else {
        const auto fingerprint = lower_ascii(identity.build_fingerprint);
        if (fingerprint.find("samsung") != std::string::npos) identity.os_family = OsFamily::OneUI;
        else if (identity.vendor == PlatformVendor::Google) identity.os_family = OsFamily::PixelOS;
        else if (identity.vendor == PlatformVendor::Vivo) identity.os_family = OsFamily::FuntouchOS;
        else if (identity.vendor == PlatformVendor::Oppo) identity.os_family = OsFamily::ColorOS;
        else if (identity.vendor == PlatformVendor::OnePlus) identity.os_family = OsFamily::OxygenOS;
        else if (!identity.android_release.empty()) identity.os_family = OsFamily::AospLike;
    }

    // Tier 1 is deliberately narrow: corroborated Xiaomi identity plus an
    // explicit HyperOS 2.x/3.x marker. MIUI, unknown versions and other OEMs
    // remain on the universal base policy.
    const bool supported_hyperos_generation =
        identity.os_family == OsFamily::HyperOS &&
        (identity.os_major == 2U || identity.os_major == 3U);
    snapshot.tier1_eligible =
        identity.vendor == PlatformVendor::Xiaomi &&
        identity.confidence == IdentityConfidence::High &&
        identity.fingerprint_vendor_match && supported_hyperos_generation;

    if (snapshot.tier1_eligible) {
        snapshot.execution_tier = ExecutionTier::XiaomiHyperOS;
        snapshot.policy_family = PolicyFamily::HyperOS4Inspired;
        snapshot.tier_selection_reason = "corroborated Xiaomi identity and explicit HyperOS 2/3 version";
    } else if (identity.vendor == PlatformVendor::Unknown) {
        snapshot.tier_selection_reason = "vendor identity unavailable; universal fallback";
    } else if (identity.vendor != PlatformVendor::Xiaomi) {
        snapshot.tier_selection_reason = "non-Xiaomi platform; universal fallback";
    } else if (identity.os_family != OsFamily::HyperOS) {
        snapshot.tier_selection_reason = "Xiaomi identity without an explicit HyperOS marker; universal fallback";
    } else if (identity.os_major != 2U && identity.os_major != 3U) {
        snapshot.tier_selection_reason = "HyperOS generation is outside the supported Tier 1 set; universal fallback";
    } else {
        snapshot.tier_selection_reason = "Xiaomi identity lacks high-confidence corroboration; universal fallback";
    }
    return snapshot;
}

} // namespace coreflow
