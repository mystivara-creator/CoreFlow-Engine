/**
 * @file coreflow_context.hpp
 * @brief Explicit composition root and deterministic runtime lifecycle owner.
 * @license MIT
 */
#pragma once

#include "coreflow/autonomous_tuner.hpp"
#include "coreflow/device_profile.hpp"
#include "coreflow/tiered_policy.hpp"
#include "coreflow/frequency_target_planner.hpp"
#include "coreflow/memory_warmup_controller.hpp"
#include "coreflow/platform_context.hpp"

#include <memory>
#include <string_view>

namespace coreflow {

class BaselineStore;
class DeviceProfiler;
class ThermalMonitor;
class NeuralPredictor;
class MutationRecoveryMonitor;
class UclampGovernor;
class FrequencyController;
class HardwareLongevityManager;
class VulkanController;

class CoreFlowContext final {
public:
    enum class Mode : uint8_t { ReadOnly = 0, Daemon = 1 };
    enum class State : uint8_t { Constructed = 0, Started = 1, Stopping = 2, Stopped = 3, Failed = 4 };

    CoreFlowContext();
    ~CoreFlowContext();

    CoreFlowContext(const CoreFlowContext&) = delete;
    CoreFlowContext& operator=(const CoreFlowContext&) = delete;
    CoreFlowContext(CoreFlowContext&&) = delete;
    CoreFlowContext& operator=(CoreFlowContext&&) = delete;

    // ReadOnly creates only the profiler/thermal inspection surface. Daemon
    // creates all mutation-capable services after durable recovery succeeds.
    [[nodiscard]] bool Start(Mode mode = Mode::Daemon) noexcept;
    [[nodiscard]] bool Stop(std::string_view reason = "CoreFlowContext shutdown") noexcept;

    [[nodiscard]] State state() const noexcept { return state_; }
    [[nodiscard]] Mode mode() const noexcept { return mode_; }
    [[nodiscard]] bool started() const noexcept { return state_ == State::Started; }
    [[nodiscard]] bool mutation_runtime_started() const noexcept {
        return state_ == State::Started && mode_ == Mode::Daemon;
    }

    [[nodiscard]] const DeviceProfileHandle& device_profile() const noexcept { return profile_; }
    [[nodiscard]] const PlatformContextSnapshot& platform_context() const noexcept { return platform_context_; }
    [[nodiscard]] DeviceProfiler& device_profiler() noexcept;
    [[nodiscard]] ThermalMonitor& thermal_monitor() noexcept;
    [[nodiscard]] NeuralPredictor& neural_predictor() noexcept;
    [[nodiscard]] CentralMutationCoordinator& mutation_coordinator() noexcept { return coordinator_; }
    [[nodiscard]] BaselineStore& baseline_store() noexcept;
    [[nodiscard]] MutationRecoveryMonitor& recovery_monitor() noexcept;
    [[nodiscard]] UclampGovernor& uclamp_governor() noexcept;
    [[nodiscard]] FrequencyController& frequency_controller() noexcept;
    [[nodiscard]] AutonomousTuner& autonomous_tuner() noexcept;
    [[nodiscard]] HardwareLongevityManager& hardware_longevity() noexcept;
    [[nodiscard]] VulkanController& vulkan_controller() noexcept;
    [[nodiscard]] TieredPolicyRouter& tiered_policy_router() noexcept { return policy_router_; }
    [[nodiscard]] FrequencyOppDiscovery& frequency_opp_discovery() noexcept { return frequency_discovery_; }
    [[nodiscard]] FrequencyTargetPlanner& frequency_target_planner() noexcept { return frequency_planner_; }
    [[nodiscard]] MemoryWarmupController& memory_warmup_controller() noexcept { return *memory_warmup_controller_; }

private:
    // Declaration order mirrors startup: reverse member destruction is a
    // safe fallback even when coordinated Stop() cannot complete recovery.
    // The coordinator outlives every callback owner; profile outlives controllers.
    CentralMutationCoordinator coordinator_;
    std::unique_ptr<ThermalMonitor> thermal_monitor_;
    std::unique_ptr<DeviceProfiler> profiler_;
    DeviceProfileHandle profile_{};
    PlatformContextSnapshot platform_context_{};
    CoreFlowBase base_policy_{};
    HyperOS4Inspired hyperos_policy_{};
    TieredPolicyRouter policy_router_;
    FrequencyOppDiscovery frequency_discovery_{"/sys"};
    FrequencyTargetPlanner frequency_planner_{};
    std::unique_ptr<MemoryWarmupController> memory_warmup_controller_;
    std::unique_ptr<MutationRecoveryMonitor> recovery_monitor_;
    std::unique_ptr<UclampGovernor> uclamp_governor_;
    std::unique_ptr<FrequencyController> frequency_controller_;
    std::unique_ptr<AutonomousTuner> autonomous_tuner_;
    std::unique_ptr<HardwareLongevityManager> hardware_longevity_;
    std::unique_ptr<NeuralPredictor> neural_predictor_;
    std::unique_ptr<VulkanController> vulkan_controller_;
    Mode mode_{Mode::ReadOnly};
    State state_{State::Constructed};

    void destroy_runtime_services() noexcept;
};

} // namespace coreflow
