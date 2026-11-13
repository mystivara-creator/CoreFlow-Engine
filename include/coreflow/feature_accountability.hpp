#pragma once
#include <string_view>
namespace coreflow {
enum class FeatureStatus { NOT_IMPLEMENTED, OBSERVATION_ONLY, LIMITED_ACTUATION };
inline constexpr std::string_view feature_status_name(FeatureStatus s) noexcept {
 switch(s){case FeatureStatus::NOT_IMPLEMENTED:return "NOT_IMPLEMENTED";case FeatureStatus::OBSERVATION_ONLY:return "OBSERVATION_ONLY";case FeatureStatus::LIMITED_ACTUATION:return "LIMITED_ACTUATION";} return "NOT_IMPLEMENTED";
}
inline constexpr FeatureStatus kArtRuntimeStatus=FeatureStatus::NOT_IMPLEMENTED,kDex2OatControlStatus=FeatureStatus::NOT_IMPLEMENTED,kJitAotControlStatus=FeatureStatus::NOT_IMPLEMENTED,kSurfaceFlingerControlStatus=FeatureStatus::NOT_IMPLEMENTED,kAngleControlStatus=FeatureStatus::NOT_IMPLEMENTED,kOpenGlControlStatus=FeatureStatus::NOT_IMPLEMENTED,kRenderBoosterStatus=FeatureStatus::LIMITED_ACTUATION,kVulkanRenderingOptimizationStatus=FeatureStatus::NOT_IMPLEMENTED,kVulkanLayerStatus=FeatureStatus::OBSERVATION_ONLY,kVulkanHardwareProbeStatus=FeatureStatus::OBSERVATION_ONLY,kQualcommKgslStatus=FeatureStatus::OBSERVATION_ONLY,kArmMaliPolicyStatus=FeatureStatus::LIMITED_ACTUATION,kPowerHalDiscoveryStatus=FeatureStatus::OBSERVATION_ONLY,kPowerHalInterceptionStatus=FeatureStatus::NOT_IMPLEMENTED;
}
