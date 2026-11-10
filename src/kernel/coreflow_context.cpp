/**
 * @file coreflow_context.cpp
 * @brief Explicit composition root and reverse-order runtime teardown.
 * @license MIT
 */
#include "coreflow/coreflow_context.hpp"
#include "coreflow/platform_context.hpp"

#include "coreflow/baseline_store.hpp"
#include "coreflow/device_profiler.hpp"
#include "coreflow/thermal_monitor.hpp"
#include "coreflow/neural_predictor.hpp"
#include "coreflow/recovery_monitor.hpp"
#include "coreflow/uclamp.hpp"
#include "coreflow/frequency_controller.hpp"
#include "coreflow/hardware_longevity.hpp"
#include "coreflow/vulkan_controller.hpp"

namespace coreflow {

CoreFlowContext::CoreFlowContext() = default;

CoreFlowContext::~CoreFlowContext() {
    (void)Stop("CoreFlowContext destructor");
}

bool CoreFlowContext::Start(Mode mode) noexcept {
    if (state_ == State::Started) return mode_ == mode;
    if (state_ == State::Stopping) return false;

    if (state_ == State::Failed || state_ == State::Stopped) {
        if (recovery_monitor_ || uclamp_governor_ || frequency_controller_ ||
            autonomous_tuner_ || hardware_longevity_ || vulkan_controller_) {
            if (!Stop("Unwind incomplete CoreFlowContext startup")) return false;
        }
    }

    mode_ = mode;
    state_ = State::Constructed;

    // Recover the durable journal before any mutation-capable owner is created.
    if (mode_ == Mode::Daemon && !baseline_store().recover_pending()) {
        state_ = State::Failed;
        return false;
    }

    thermal_monitor_ = std::make_unique<ThermalMonitor>();
    profiler_ = std::make_unique<DeviceProfiler>();
    profile_ = profiler_->refresh();
    const NativeAndroidPropertySource native_properties;
    const PlatformContextDetector platform_detector(native_properties);
    platform_context_ = platform_detector.detect(profile_->generation);
    if (!profile_) {
        destroy_runtime_services();
        state_ = State::Failed;
        return false;
    }

    if (mode_ == Mode::ReadOnly) {
        // No mutation callback, actuator, tuner, predictor training, or worker
        // is started for short-lived CLI inspection commands.
        state_ = State::Started;
        return true;
    }

    recovery_monitor_ = std::make_unique<MutationRecoveryMonitor>(baseline_store(), coordinator_);
    uclamp_governor_ = std::make_unique<UclampGovernor>(coordinator_);
    frequency_controller_ = std::make_unique<FrequencyController>(coordinator_, "/sys", profile_);
    autonomous_tuner_ = std::make_unique<AutonomousTuner>(coordinator_, "/sys", "/proc", profile_);
    hardware_longevity_ = std::make_unique<HardwareLongevityManager>(coordinator_);
    neural_predictor_ = std::make_unique<NeuralPredictor>();
    vulkan_controller_ = std::make_unique<VulkanController>(coordinator_);

    state_ = State::Started;
    return true;
}

bool CoreFlowContext::Stop(std::string_view reason) noexcept {
    if (state_ == State::Constructed || state_ == State::Stopped) {
        destroy_runtime_services();
        profile_.reset();
        platform_context_ = {};
        state_ = State::Stopped;
        return true;
    }
    if (state_ == State::Stopping) return false;

    state_ = State::Stopping;
    if (mode_ == Mode::Daemon &&
        (recovery_monitor_ || uclamp_governor_ || frequency_controller_ ||
         autonomous_tuner_ || hardware_longevity_ || vulkan_controller_)) {
        const RecoveryResult result = coordinated_rollback(
            coordinator_, baseline_store(), reason);
        if (!result.recovered()) {
            // Keep callback owners alive for another recovery attempt. If the
            // process is terminating, their destructors still run before the
            // coordinator and retain any unresolved durable journal entries.
            state_ = State::Failed;
            return false;
        }
    }

    destroy_runtime_services();
    profile_.reset();
    platform_context_ = {};
    state_ = State::Stopped;
    return true;
}

void CoreFlowContext::destroy_runtime_services() noexcept {
    // Reverse dependency order: callback owners die while coordinator_ is alive.
    vulkan_controller_.reset();
    neural_predictor_.reset();
    hardware_longevity_.reset();
    autonomous_tuner_.reset();
    frequency_controller_.reset();
    uclamp_governor_.reset();
    recovery_monitor_.reset();
    profiler_.reset();
    thermal_monitor_.reset();
}

DeviceProfiler& CoreFlowContext::device_profiler() noexcept { return *profiler_; }
ThermalMonitor& CoreFlowContext::thermal_monitor() noexcept { return *thermal_monitor_; }
NeuralPredictor& CoreFlowContext::neural_predictor() noexcept { return *neural_predictor_; }
BaselineStore& CoreFlowContext::baseline_store() noexcept { return BaselineStore::instance(); }
MutationRecoveryMonitor& CoreFlowContext::recovery_monitor() noexcept { return *recovery_monitor_; }
UclampGovernor& CoreFlowContext::uclamp_governor() noexcept { return *uclamp_governor_; }
FrequencyController& CoreFlowContext::frequency_controller() noexcept { return *frequency_controller_; }
AutonomousTuner& CoreFlowContext::autonomous_tuner() noexcept { return *autonomous_tuner_; }
HardwareLongevityManager& CoreFlowContext::hardware_longevity() noexcept { return *hardware_longevity_; }
VulkanController& CoreFlowContext::vulkan_controller() noexcept { return *vulkan_controller_; }

} // namespace coreflow
