#include "coreflow/feature_accountability.hpp"
#include "coreflow/render_booster.hpp"
#include "coreflow/power_bridge.hpp"
#include "coreflow/vulkan_controller.hpp"
using namespace coreflow;
static_assert(kArtRuntimeStatus==FeatureStatus::NOT_IMPLEMENTED);
static_assert(kDex2OatControlStatus==FeatureStatus::NOT_IMPLEMENTED);
static_assert(kJitAotControlStatus==FeatureStatus::NOT_IMPLEMENTED);
static_assert(kSurfaceFlingerControlStatus==FeatureStatus::NOT_IMPLEMENTED);
static_assert(kAngleControlStatus==FeatureStatus::NOT_IMPLEMENTED);
static_assert(kOpenGlControlStatus==FeatureStatus::NOT_IMPLEMENTED);
static_assert(RenderBooster::kStatus==FeatureStatus::LIMITED_ACTUATION);
static_assert(VulkanController::kRenderingOptimizationStatus==FeatureStatus::NOT_IMPLEMENTED);
static_assert(VulkanController::kLayerStatus==FeatureStatus::OBSERVATION_ONLY);
static_assert(VulkanController::kHardwareProbeStatus==FeatureStatus::OBSERVATION_ONLY);
static_assert(VulkanController::kQualcommKgslFeatureStatus==FeatureStatus::LIMITED_ACTUATION);
static_assert(kQualcommKgslStatus==FeatureStatus::LIMITED_ACTUATION);
static_assert(VulkanController::kArmMaliPolicyFeatureStatus==FeatureStatus::LIMITED_ACTUATION);
static_assert(PowerBridge::kDiscoveryStatus==FeatureStatus::OBSERVATION_ONLY);
static_assert(PowerBridge::kInterceptionStatus==FeatureStatus::NOT_IMPLEMENTED);
int main(){return feature_status_name(FeatureStatus::NOT_IMPLEMENTED)=="NOT_IMPLEMENTED" && feature_status_name(FeatureStatus::OBSERVATION_ONLY)=="OBSERVATION_ONLY" && feature_status_name(FeatureStatus::LIMITED_ACTUATION)=="LIMITED_ACTUATION" ? 0:1;}
