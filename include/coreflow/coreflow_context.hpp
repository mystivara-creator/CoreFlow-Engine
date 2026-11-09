/**
 * @file coreflow_context.hpp
 * @brief Explicit composition root and deterministic runtime lifecycle owner.
 * @license MIT
 */
#pragma once

#include "coreflow/autonomous_tuner.hpp"
#include "coreflow/device_profile.hpp"

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

private:
    // Declaration order mirrors startup: reverse member destruction is a
    // safe fallback even when coordinated Stop() cannot complete recovery.
    // The coordinator outlives every callback owner; profile outlives controllers.
    CentralMutationCoordinator coordinator_;
    std::unique_ptr<ThermalMonitor> thermal_monitor_;
    std::unique_ptr<DeviceProfiler> profiler_;
    DeviceProfileHandle profile_{};
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
