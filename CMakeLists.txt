cmake_minimum_required(VERSION 3.22.1)
project(CoreFlowEngine VERSION 3.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CORE_FLOW_BUILD_TESTS "Build CoreFlow unit test suite" ON)

# Optimization & Security Hardening Flags for Native Systems
add_compile_options(
    -fvisibility=hidden
    -fvisibility-inlines-hidden
    -Wall
    -Wextra
    -Wpedantic
    -fno-exceptions
    -fno-rtti
    -fstack-protector-strong
    -Wformat
    -Wformat-security
    -Werror=return-type
)

add_compile_definitions(_FORTIFY_SOURCE=2)

add_link_options(
    -Wl,--gc-sections
    $<$<CONFIG:Release>:-Wl,--strip-all>
    -Wl,-z,relro
    -Wl,-z,now
)

include_directories(include)
include(cmake/RustEngine.cmake)

# Modern CoreFlow Kernel & System Sources
set(COREFLOW_KERNEL_SOURCES
    src/kernel/cgroup_v2.cpp
    src/kernel/uclamp.cpp
    src/kernel/baseline_store.cpp
    src/kernel/mutation_coordinator.cpp
    src/kernel/recovery_monitor.cpp
    src/kernel/render_booster.cpp
    src/kernel/device_profiler.cpp
    src/kernel/thermal_monitor.cpp
    src/kernel/hardware_longevity.cpp
    src/kernel/frequency_controller.cpp
    src/kernel/power_bridge.cpp
    src/kernel/neural_predictor.cpp
    src/kernel/autonomous_tuner.cpp
    src/kernel/vulkan_controller.cpp
)

# Universal Vulkan 1.4 Implicit Layer (libVkLayer_coreflow.so)
add_library(VkLayer_coreflow SHARED
    src/layer/vk_layer_coreflow.cpp
)
target_compile_options(VkLayer_coreflow PRIVATE $<$<CONFIG:Release>:-O2>)
if(ANDROID)
    target_link_libraries(VkLayer_coreflow PRIVATE log)
else()
    target_link_libraries(VkLayer_coreflow PRIVATE pthread)
endif()

# Main Daemon (coreflowd)
add_executable(coreflowd
    src/daemon/main.cpp
    src/daemon/reactor.cpp
    ${COREFLOW_KERNEL_SOURCES}
)
target_link_libraries(coreflowd PRIVATE coreflow_rust ${CMAKE_DL_LIBS} m)
target_compile_options(coreflowd PRIVATE $<$<CONFIG:Release>:-O2>)

if(ANDROID)
    target_link_libraries(coreflowd PRIVATE log)
else()
    target_link_libraries(coreflowd PRIVATE pthread)
endif()

# Unit Test Target
if(CORE_FLOW_BUILD_TESTS)
    enable_testing()
    add_executable(test_autonomous_tuner tests/test_autonomous_tuner.cpp src/kernel/autonomous_tuner.cpp src/kernel/mutation_coordinator.cpp src/kernel/baseline_store.cpp)
    target_link_libraries(test_autonomous_tuner PRIVATE m)
    add_test(NAME AutonomousTunerTest COMMAND test_autonomous_tuner)

    add_executable(test_vulkan_controller tests/test_vulkan_controller.cpp src/kernel/vulkan_controller.cpp src/kernel/mutation_coordinator.cpp src/kernel/baseline_store.cpp)
    target_link_libraries(test_vulkan_controller PRIVATE VkLayer_coreflow ${CMAKE_DL_LIBS} m)
    add_test(NAME VulkanControllerTest COMMAND test_vulkan_controller)

    add_executable(test_neural_predictor tests/test_neural_predictor.cpp src/kernel/neural_predictor.cpp)
    target_link_libraries(test_neural_predictor PRIVATE ${CMAKE_DL_LIBS} m)
    add_test(NAME NeuralPredictorTest COMMAND test_neural_predictor)

    add_executable(test_frequency_controller tests/test_frequency_controller.cpp src/kernel/frequency_controller.cpp src/kernel/mutation_coordinator.cpp src/kernel/baseline_store.cpp)
    target_link_libraries(test_frequency_controller PRIVATE m)
    add_test(NAME FrequencyOwnershipTest COMMAND test_frequency_controller)

    add_executable(test_cgroup_v2
        tests/test_cgroup_v2.cpp
        src/kernel/cgroup_v2.cpp
        src/kernel/frequency_controller.cpp
        src/kernel/mutation_coordinator.cpp
        src/kernel/baseline_store.cpp
    )
    target_include_directories(test_cgroup_v2 PRIVATE include)
    target_link_libraries(test_cgroup_v2 PRIVATE m)
    if(ANDROID)
        target_link_libraries(test_cgroup_v2 PRIVATE log)
    endif()
    add_test(NAME CgroupV2OwnershipTest COMMAND test_cgroup_v2)

    add_executable(test_longevity tests/test_longevity.cpp src/kernel/hardware_longevity.cpp src/kernel/mutation_coordinator.cpp src/kernel/baseline_store.cpp)
    target_link_libraries(test_longevity PRIVATE m)
    add_test(NAME ChargingOwnershipTest COMMAND test_longevity)

    add_executable(test_recovery_monitor
        tests/test_recovery_monitor.cpp
        src/kernel/recovery_monitor.cpp
        src/kernel/autonomous_tuner.cpp
        src/kernel/hardware_longevity.cpp
        src/kernel/mutation_coordinator.cpp
        src/kernel/baseline_store.cpp
    )
    target_link_libraries(test_recovery_monitor PRIVATE m pthread)
    add_test(NAME RecoveryMonitorTest COMMAND test_recovery_monitor)

    add_executable(test_rust_bridge tests/test_rust_bridge.cpp)
    target_link_libraries(test_rust_bridge PRIVATE coreflow_rust ${CMAKE_DL_LIBS} m)
    if(NOT ANDROID)
        target_link_libraries(test_rust_bridge PRIVATE pthread)
    endif()
    add_test(NAME RustBridgeIntegrationTest COMMAND test_rust_bridge)

    add_executable(test_baseline_rollback
        tests/test_baseline_rollback.cpp
        src/kernel/baseline_store.cpp
    )
    target_include_directories(test_baseline_rollback PRIVATE include)
    target_link_libraries(test_baseline_rollback PRIVATE m)
    if(ANDROID)
        target_link_libraries(test_baseline_rollback PRIVATE log)
    endif()
    add_test(NAME BaselineRollbackTest COMMAND test_baseline_rollback)

    add_executable(test_config
        tests/test_config.cpp
    )
    target_include_directories(test_config PRIVATE include)
    if(ANDROID)
        target_link_libraries(test_config PRIVATE log)
    endif()
    add_test(NAME ConfigTest COMMAND test_config)

    add_executable(test_stability_policy
        tests/test_stability_policy.cpp
    )
    target_include_directories(test_stability_policy PRIVATE include)
    target_link_libraries(test_stability_policy PRIVATE m)
    if(ANDROID)
        target_link_libraries(test_stability_policy PRIVATE log)
    endif()
    add_test(NAME StabilityPolicyTest COMMAND test_stability_policy)

    add_executable(test_thermal_monitor
        tests/test_thermal_monitor.cpp
        src/kernel/thermal_monitor.cpp
    )
    target_include_directories(test_thermal_monitor PRIVATE include)
    target_link_libraries(test_thermal_monitor PRIVATE m)
    if(ANDROID)
        target_link_libraries(test_thermal_monitor PRIVATE log)
    endif()
    add_test(NAME ThermalMonitorTest COMMAND test_thermal_monitor)

    add_executable(test_uclamp
        tests/test_uclamp.cpp
        src/kernel/uclamp.cpp
        src/kernel/mutation_coordinator.cpp
        src/kernel/baseline_store.cpp
    )
    target_include_directories(test_uclamp PRIVATE include)
    target_link_libraries(test_uclamp PRIVATE m)
    if(ANDROID)
        target_link_libraries(test_uclamp PRIVATE log)
    endif()
    add_test(NAME UclampTest COMMAND test_uclamp)

    if(NOT ANDROID)
        add_test(
            NAME ModuleLifecycleTest
            COMMAND /bin/sh ${CMAKE_SOURCE_DIR}/tests/test_module_lifecycle.sh
        )
    endif()
endif()

