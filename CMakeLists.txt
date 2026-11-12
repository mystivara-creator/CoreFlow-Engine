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

execute_process(
    COMMAND git rev-parse --short=12 HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE COREFLOW_BUILD_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT COREFLOW_BUILD_COMMIT MATCHES "^[a-f0-9]+$")
    set(COREFLOW_BUILD_COMMIT "unknown")
endif()

# Modern CoreFlow Kernel & System Sources
set(COREFLOW_KERNEL_SOURCES
    src/kernel/coreflow_context.cpp
    src/kernel/platform_context.cpp
    src/kernel/workload_intelligence.cpp
    src/kernel/tiered_policy.cpp
    src/kernel/frequency_target_planner.cpp
    src/kernel/transactional_frequency_actuator.cpp
    src/kernel/memory_warmup_controller.cpp
    src/kernel/cgroup_v2.cpp
    src/kernel/mount_topology.cpp
    src/kernel/uclamp.cpp
    src/kernel/baseline_store.cpp
    src/kernel/mutation_coordinator.cpp
    src/kernel/recovery_monitor.cpp
    src/kernel/render_booster.cpp
    src/kernel/frame_bridge.cpp
    src/kernel/input_event_source.cpp
    src/kernel/foreground_task_resolver.cpp
    src/kernel/device_profiler.cpp
    src/kernel/kernel_node_inventory.cpp
    src/kernel/intent_actuator.cpp
    src/mutation/intent_actuator.cpp
    src/semantic/domain_evidence_aggregator.cpp
    src/semantic/capability_resolver.cpp
    src/semantic/semantic_evidence_serializer.cpp
    src/semantic/linux_sysfs_discovery_adapter.cpp
    src/platform/discovery_adapter.cpp
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
    src/daemon/device_validation.cpp
    ${COREFLOW_KERNEL_SOURCES}
)
target_link_libraries(coreflowd PRIVATE coreflow_rust ${CMAKE_DL_LIBS} m)
target_compile_options(coreflowd PRIVATE $<$<CONFIG:Release>:-O2>)
target_compile_definitions(coreflowd PRIVATE COREFLOW_BUILD_COMMIT="${COREFLOW_BUILD_COMMIT}")

if(ANDROID)
    target_link_libraries(coreflowd PRIVATE log)
else()
    target_link_libraries(coreflowd PRIVATE pthread)
endif()

# Unit Test Target
if(CORE_FLOW_BUILD_TESTS)
    enable_testing()
    add_executable(test_mount_topology
        tests/test_mount_topology.cpp
        src/kernel/mount_topology.cpp
    )
    target_include_directories(test_mount_topology PRIVATE include)
    add_test(NAME MountTopologyDynamicDiscoveryTest COMMAND test_mount_topology)

    add_executable(test_kernel_node_inventory
        tests/test_kernel_node_inventory.cpp
        src/kernel/kernel_node_inventory.cpp
    )
    target_include_directories(test_kernel_node_inventory PRIVATE include)
    add_test(NAME KernelNodeInventoryCoAdaptationTest COMMAND test_kernel_node_inventory)

    add_executable(test_cddp
        tests/test_cddp.cpp
        src/kernel/device_profiler.cpp
        src/kernel/kernel_node_inventory.cpp
        src/kernel/thermal_monitor.cpp
        src/kernel/frequency_controller.cpp
        src/kernel/autonomous_tuner.cpp
        src/kernel/foreground_task_resolver.cpp
        src/kernel/cgroup_v2.cpp
        src/kernel/mount_topology.cpp
        src/kernel/transactional_frequency_actuator.cpp
        src/kernel/frequency_target_planner.cpp
        src/kernel/tiered_policy.cpp
        src/kernel/platform_context.cpp
        src/kernel/mutation_coordinator.cpp
        src/kernel/baseline_store.cpp
    )
    target_include_directories(test_cddp PRIVATE include tests)
    target_link_libraries(test_cddp PRIVATE m)
    if(NOT ANDROID)
        target_link_libraries(test_cddp PRIVATE pthread)
    endif()
    if(ANDROID)
        target_link_libraries(test_cddp PRIVATE log)
    else()
        target_link_libraries(test_cddp PRIVATE pthread)
    endif()
    add_test(NAME CoreFlowDynamicDeviceProfilingTest COMMAND test_cddp)

    add_executable(test_p3_intent_actuator
        tests/test_p3_intent_actuator.cpp
        src/kernel/device_profiler.cpp
        src/kernel/kernel_node_inventory.cpp
        src/kernel/mount_topology.cpp
        src/kernel/intent_actuator.cpp
        src/mutation/intent_actuator.cpp
        src/kernel/thermal_monitor.cpp
        src/kernel/mutation_coordinator.cpp
        src/kernel/baseline_store.cpp
    )
    target_include_directories(test_p3_intent_actuator PRIVATE include)
    target_link_libraries(test_p3_intent_actuator PRIVATE m)
    if(NOT ANDROID)
        target_link_libraries(test_p3_intent_actuator PRIVATE pthread)
    endif()
    if(ANDROID)
        target_link_libraries(test_p3_intent_actuator PRIVATE log)
    endif()
    add_test(NAME P3DynamicInfrastructureActuatorTest COMMAND test_p3_intent_actuator)

    add_executable(test_capability_resolver
        tests/semantic/test_capability_resolver.cpp
        src/semantic/capability_resolver.cpp
        src/semantic/domain_evidence_aggregator.cpp
        src/platform/discovery_adapter.cpp
    )
    target_include_directories(test_capability_resolver PRIVATE include)
    if(NOT ANDROID)
        target_link_libraries(test_capability_resolver PRIVATE pthread)
    endif()
    add_test(NAME P3CapabilityResolverTest COMMAND test_capability_resolver)

    add_executable(test_linux_sysfs_discovery_adapter
        tests/semantic/test_linux_sysfs_discovery_adapter.cpp
        src/semantic/linux_sysfs_discovery_adapter.cpp
        src/kernel/mount_topology.cpp
        src/semantic/capability_resolver.cpp
        src/platform/discovery_adapter.cpp
        src/semantic/domain_evidence_aggregator.cpp
    )
    target_include_directories(test_linux_sysfs_discovery_adapter PRIVATE include)
    add_test(NAME LinuxSysfsDiscoveryAdapterTest COMMAND test_linux_sysfs_discovery_adapter)

    add_executable(test_domain_evidence_aggregator
        tests/semantic/test_domain_evidence_aggregator.cpp
        src/semantic/domain_evidence_aggregator.cpp
    )
    target_include_directories(test_domain_evidence_aggregator PRIVATE include)
    add_test(NAME P3FunctionalDomainEvidenceTest COMMAND test_domain_evidence_aggregator)

    add_executable(test_semantic_evidence_serializer
        tests/semantic/test_semantic_evidence_serializer.cpp
        src/semantic/semantic_evidence_serializer.cpp
        src/semantic/capability_resolver.cpp
        src/semantic/domain_evidence_aggregator.cpp
        src/platform/discovery_adapter.cpp
    )
    target_include_directories(test_semantic_evidence_serializer PRIVATE include)
    if(NOT ANDROID)
        target_link_libraries(test_semantic_evidence_serializer PRIVATE pthread)
    endif()
    add_test(NAME P3SemanticEvidenceSerializationTest COMMAND test_semantic_evidence_serializer)

    add_executable(test_platform_context
        tests/test_platform_context.cpp
        src/kernel/platform_context.cpp
        src/kernel/workload_intelligence.cpp
    )
    target_include_directories(test_platform_context PRIVATE include)
    add_test(NAME PlatformContextTierDetectionTest COMMAND test_platform_context)

    add_executable(test_p2d_policy
        tests/test_p2d_policy.cpp
        src/kernel/tiered_policy.cpp
        src/kernel/frequency_target_planner.cpp
    )
    target_include_directories(test_p2d_policy PRIVATE include)
    target_link_libraries(test_p2d_policy PRIVATE m)
    add_test(NAME P2DTieredPolicyAndFrequencyPlannerTest COMMAND test_p2d_policy)

    add_executable(test_memory_warmup
        tests/test_memory_warmup.cpp
        src/kernel/memory_warmup_controller.cpp
    )
    target_include_directories(test_memory_warmup PRIVATE include)
    add_test(NAME P2DMemoryWarmupControllerTest COMMAND test_memory_warmup)

    add_executable(test_transactional_frequency_actuator
        tests/test_transactional_frequency_actuator.cpp
        src/kernel/transactional_frequency_actuator.cpp
        src/kernel/frequency_target_planner.cpp
        src/kernel/thermal_monitor.cpp
        src/kernel/mutation_coordinator.cpp
        src/kernel/baseline_store.cpp
    )
    target_include_directories(test_transactional_frequency_actuator PRIVATE include)
    target_link_libraries(test_transactional_frequency_actuator PRIVATE m)
    if(NOT ANDROID)
        target_link_libraries(test_transactional_frequency_actuator PRIVATE pthread)
    endif()
    if(ANDROID)
        target_link_libraries(test_transactional_frequency_actuator PRIVATE log)
    endif()
    add_test(NAME P2DTransactionalFrequencyActuatorTest COMMAND test_transactional_frequency_actuator)

    add_executable(test_p2d6_provider_ownership
        tests/test_p2d6_provider_ownership.cpp
        src/kernel/transactional_frequency_actuator.cpp
        src/kernel/frequency_target_planner.cpp
        src/kernel/thermal_monitor.cpp
        src/kernel/mutation_coordinator.cpp
        src/kernel/baseline_store.cpp
    )
    target_include_directories(test_p2d6_provider_ownership PRIVATE include)
    target_link_libraries(test_p2d6_provider_ownership PRIVATE m)
    if(NOT ANDROID)
        target_link_libraries(test_p2d6_provider_ownership PRIVATE pthread)
    endif()
    if(ANDROID)
        target_link_libraries(test_p2d6_provider_ownership PRIVATE log)
    endif()
    add_test(NAME P2D6GenericProviderAndOwnershipGuardTest COMMAND test_p2d6_provider_ownership)

    add_executable(test_autonomous_tuner
        tests/test_autonomous_tuner.cpp
        src/kernel/autonomous_tuner.cpp
        src/kernel/foreground_task_resolver.cpp
        src/kernel/cgroup_v2.cpp
        src/kernel/mount_topology.cpp
        src/kernel/transactional_frequency_actuator.cpp
        src/kernel/frequency_target_planner.cpp
        src/kernel/thermal_monitor.cpp
        src/kernel/tiered_policy.cpp
        src/kernel/platform_context.cpp
        src/kernel/mutation_coordinator.cpp
        src/kernel/baseline_store.cpp
    )
    target_link_libraries(test_autonomous_tuner PRIVATE m)
    if(NOT ANDROID)
        target_link_libraries(test_autonomous_tuner PRIVATE pthread)
    endif()
    add_test(NAME AutonomousTunerTest COMMAND test_autonomous_tuner)

    add_executable(test_feature_accountability tests/test_feature_accountability.cpp)
    target_include_directories(test_feature_accountability PRIVATE include)
    add_test(NAME FeatureAccountabilityTest COMMAND test_feature_accountability)

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
        src/kernel/mount_topology.cpp
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

    add_executable(test_foreground_task_resolver
        tests/test_foreground_task_resolver.cpp
        src/kernel/foreground_task_resolver.cpp
        src/kernel/cgroup_v2.cpp
        src/kernel/mount_topology.cpp
        src/kernel/frequency_controller.cpp
        src/kernel/mutation_coordinator.cpp
        src/kernel/baseline_store.cpp
    )
    target_include_directories(test_foreground_task_resolver PRIVATE include)
    target_link_libraries(test_foreground_task_resolver PRIVATE m)
    if(NOT ANDROID)
        target_link_libraries(test_foreground_task_resolver PRIVATE pthread)
    else()
        target_link_libraries(test_foreground_task_resolver PRIVATE log)
    endif()
    add_test(NAME ForegroundTaskResolverTest COMMAND test_foreground_task_resolver)

    add_executable(test_longevity tests/test_longevity.cpp src/kernel/hardware_longevity.cpp src/kernel/mount_topology.cpp src/kernel/mutation_coordinator.cpp src/kernel/baseline_store.cpp)
    target_link_libraries(test_longevity PRIVATE m)
    add_test(NAME ChargingOwnershipTest COMMAND test_longevity)

    add_executable(test_recovery_monitor
        tests/test_recovery_monitor.cpp
        src/daemon/reactor.cpp
        src/kernel/recovery_monitor.cpp
        src/kernel/autonomous_tuner.cpp
        src/kernel/foreground_task_resolver.cpp
        src/kernel/cgroup_v2.cpp
        src/kernel/mount_topology.cpp
        src/kernel/transactional_frequency_actuator.cpp
        src/kernel/frequency_target_planner.cpp
        src/kernel/thermal_monitor.cpp
        src/kernel/tiered_policy.cpp
        src/kernel/platform_context.cpp
        src/kernel/hardware_longevity.cpp
        src/kernel/mutation_coordinator.cpp
        src/kernel/baseline_store.cpp
    )
    target_link_libraries(test_recovery_monitor PRIVATE m)
    if(NOT ANDROID)
        target_link_libraries(test_recovery_monitor PRIVATE pthread)
    endif()
    add_test(NAME RecoveryMonitorTest COMMAND test_recovery_monitor)

    add_executable(test_coreflow_context
        tests/test_coreflow_context.cpp
        src/daemon/reactor.cpp
        ${COREFLOW_KERNEL_SOURCES}
    )
    target_link_libraries(test_coreflow_context PRIVATE VkLayer_coreflow ${CMAKE_DL_LIBS} m)
    if(NOT ANDROID)
        target_link_libraries(test_coreflow_context PRIVATE pthread)
    endif()
    add_test(NAME CoreFlowContextLifecycleTest COMMAND test_coreflow_context)

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

    add_executable(test_device_validation
        tests/test_device_validation.cpp
        src/daemon/device_validation.cpp
    )
    target_include_directories(test_device_validation PRIVATE include)
    target_compile_definitions(test_device_validation PRIVATE COREFLOW_BUILD_COMMIT="${COREFLOW_BUILD_COMMIT}")
    if(NOT ANDROID)
        target_link_libraries(test_device_validation PRIVATE pthread)
    endif()
    add_test(NAME DeviceValidationRuntimeTest COMMAND test_device_validation)

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
        src/kernel/mount_topology.cpp
        src/kernel/mutation_coordinator.cpp
        src/kernel/baseline_store.cpp
    )
    target_include_directories(test_uclamp PRIVATE include)
    target_link_libraries(test_uclamp PRIVATE m)
    if(ANDROID)
        target_link_libraries(test_uclamp PRIVATE log)
    endif()
    add_test(NAME UclampTest COMMAND test_uclamp)

    add_executable(test_render_booster
        tests/test_render_booster.cpp
        src/kernel/render_booster.cpp
        src/kernel/frame_bridge.cpp
        src/daemon/reactor.cpp
        src/kernel/uclamp.cpp
        src/kernel/mount_topology.cpp
        src/kernel/mutation_coordinator.cpp
        src/kernel/baseline_store.cpp
    )
    target_include_directories(test_render_booster PRIVATE include)
    target_link_libraries(test_render_booster PRIVATE m)
    if(NOT ANDROID)
        target_link_libraries(test_render_booster PRIVATE pthread)
    endif()
    if(ANDROID)
        target_link_libraries(test_render_booster PRIVATE log)
    endif()
    add_test(NAME RenderBoosterOwnershipAndExpiryTest COMMAND test_render_booster)

    add_executable(test_input_event_source
        tests/test_input_event_source.cpp
        src/kernel/input_event_source.cpp
        src/daemon/reactor.cpp
        src/kernel/foreground_task_resolver.cpp
        src/kernel/cgroup_v2.cpp
        src/kernel/mount_topology.cpp
        src/kernel/frequency_controller.cpp
        src/kernel/mutation_coordinator.cpp
        src/kernel/baseline_store.cpp
    )
    target_include_directories(test_input_event_source PRIVATE include)
    target_link_libraries(test_input_event_source PRIVATE m)
    if(NOT ANDROID)
        target_link_libraries(test_input_event_source PRIVATE pthread)
    else()
        target_link_libraries(test_input_event_source PRIVATE log)
    endif()
    add_test(NAME NativeInputEventSourceTest COMMAND test_input_event_source)

    if(NOT ANDROID)
        find_package(Python3 COMPONENTS Interpreter QUIET)
        if(Python3_Interpreter_FOUND)
            add_test(
                NAME DeviceValidationBundleTest
                COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_cf_dvs_validator.py
            )
            add_test(
                NAME DeviceValidationRuntimeContractTest
                COMMAND ${Python3_EXECUTABLE}
                    ${CMAKE_SOURCE_DIR}/tests/test_cf_dvs_runtime_bundle.py
                    $<TARGET_FILE:test_device_validation>
            )

            add_test(
                NAME CliOneShotLifecycleContractTest
                COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_cli_lifecycle.py
            )
        endif()


        add_test(
            NAME ModuleLifecycleTest
            COMMAND /bin/sh ${CMAKE_SOURCE_DIR}/tests/test_module_lifecycle.sh
        )
    endif()
endif()

