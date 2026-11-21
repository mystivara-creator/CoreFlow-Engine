cmake_minimum_required(VERSION 3.22.1)
project(CoreFlowEngine VERSION 6.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CORE_FLOW_BUILD_TESTS "Build CoreFlow unit test suite" ON)

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

# CoreFlow v6.1 Autonomous Co-Pilot & 4-Pillar Sources
set(COREFLOW_V5_SOURCES
    src/v5/hardware_probe.cpp
    src/v5/context_engine.cpp
    src/v5/thread_scheduler.cpp
    src/v5/storage_memory_actuator.cpp
    src/v5/thermal_envelope_actuator.cpp
    src/v6/oem_collaboration_map.cpp
    src/v6/semantic_knob_engine.cpp
    src/kernel/baseline_store.cpp
    src/kernel/mutation_coordinator.cpp
    src/kernel/mount_topology.cpp
    src/kernel/thermal_monitor.cpp
    src/kernel/hardware_longevity.cpp
    src/kernel/neural_predictor.cpp
    src/kernel/frame_bridge.cpp
    src/kernel/input_event_source.cpp
)

add_executable(coreflowd
    src/daemon/main.cpp
    src/daemon/reactor.cpp
    ${COREFLOW_V5_SOURCES}
)
target_link_libraries(coreflowd PRIVATE coreflow_rust ${CMAKE_DL_LIBS} m)
target_compile_options(coreflowd PRIVATE $<$<CONFIG:Release>:-O2>)
target_compile_definitions(coreflowd PRIVATE COREFLOW_BUILD_COMMIT="${COREFLOW_BUILD_COMMIT}")

if(ANDROID)
    target_link_libraries(coreflowd PRIVATE log)
else()
    target_link_libraries(coreflowd PRIVATE pthread)
endif()

if(CORE_FLOW_BUILD_TESTS)
    enable_testing()
    add_compile_options(-UNDEBUG)

    # CoreFlow v5/v6.1 Unit Tests
    add_executable(test_v5_hardware_probe
        tests/test_v5_hardware_probe.cpp
        src/v5/hardware_probe.cpp
        src/kernel/mount_topology.cpp
    )
    add_test(NAME V5HardwareProbeTest COMMAND test_v5_hardware_probe)

    add_executable(test_v5_context_engine
        tests/test_v5_context_engine.cpp
        src/v5/context_engine.cpp
    )
    add_test(NAME V5ContextEngineTest COMMAND test_v5_context_engine)

    add_executable(test_v5_thread_scheduler
        tests/test_v5_thread_scheduler.cpp
        src/v5/thread_scheduler.cpp
    )
    target_link_libraries(test_v5_thread_scheduler PRIVATE coreflow_rust m)
    add_test(NAME V5ThreadSchedulerTest COMMAND test_v5_thread_scheduler)

    add_executable(test_v5_storage_memory_actuator
        tests/test_v5_storage_memory_actuator.cpp
        src/v5/storage_memory_actuator.cpp
        src/v6/oem_collaboration_map.cpp
        src/kernel/baseline_store.cpp
    )
    if(NOT ANDROID)
        target_link_libraries(test_v5_storage_memory_actuator PRIVATE pthread)
    endif()
    add_test(NAME V5StorageMemoryActuatorTest COMMAND test_v5_storage_memory_actuator)

    add_executable(test_v5_thermal_envelope
        tests/test_v5_thermal_envelope.cpp
        src/v5/thermal_envelope_actuator.cpp
        src/v6/oem_collaboration_map.cpp
        src/kernel/baseline_store.cpp
    )
    if(NOT ANDROID)
        target_link_libraries(test_v5_thermal_envelope PRIVATE pthread)
    endif()
    add_test(NAME V5ThermalEnvelopeActuatorTest COMMAND test_v5_thermal_envelope)

    add_executable(test_v6_decision_journal
        tests/test_v6_decision_journal.cpp
    )
    if(NOT ANDROID)
        target_link_libraries(test_v6_decision_journal PRIVATE pthread)
    endif()
    add_test(NAME V6DecisionJournalTest COMMAND test_v6_decision_journal)

    add_executable(test_v6_semantic_copilot
        tests/test_v6_semantic_copilot.cpp
        src/v6/oem_collaboration_map.cpp
        src/v6/semantic_knob_engine.cpp
        src/v5/hardware_probe.cpp
        src/kernel/mount_topology.cpp
        src/kernel/baseline_store.cpp
    )
    if(NOT ANDROID)
        target_link_libraries(test_v6_semantic_copilot PRIVATE pthread)
    endif()
    add_test(NAME V6SemanticCopilotTest COMMAND test_v6_semantic_copilot)

    # Foundational Infrastructure & Safety Tests
    add_executable(test_mount_topology
        tests/test_mount_topology.cpp
        src/kernel/mount_topology.cpp
    )
    add_test(NAME MountTopologyDynamicDiscoveryTest COMMAND test_mount_topology)

    add_executable(test_neural_predictor
        tests/test_neural_predictor.cpp
        src/kernel/neural_predictor.cpp
    )
    target_link_libraries(test_neural_predictor PRIVATE ${CMAKE_DL_LIBS} m)
    add_test(NAME NeuralPredictorTest COMMAND test_neural_predictor)

    add_executable(test_longevity
        tests/test_longevity.cpp
        src/kernel/hardware_longevity.cpp
        src/kernel/mount_topology.cpp
        src/kernel/mutation_coordinator.cpp
        src/kernel/baseline_store.cpp
    )
    if(NOT ANDROID)
        target_link_libraries(test_longevity PRIVATE pthread)
    endif()
    add_test(NAME ChargingOwnershipTest COMMAND test_longevity)

    add_executable(test_rust_bridge tests/test_rust_bridge.cpp)
    target_link_libraries(test_rust_bridge PRIVATE coreflow_rust m)
    add_test(NAME RustBridgeIntegrationTest COMMAND test_rust_bridge)

    add_executable(test_baseline_rollback
        tests/test_baseline_rollback.cpp
        src/kernel/baseline_store.cpp
    )
    if(NOT ANDROID)
        target_link_libraries(test_baseline_rollback PRIVATE pthread)
    endif()
    add_test(NAME BaselineRollbackTest COMMAND test_baseline_rollback)

    add_executable(test_config tests/test_config.cpp)
    add_test(NAME ConfigTest COMMAND test_config)

    add_executable(test_stability_policy tests/test_stability_policy.cpp)
    add_test(NAME StabilityPolicyTest COMMAND test_stability_policy)

    add_executable(test_thermal_monitor
        tests/test_thermal_monitor.cpp
        src/kernel/thermal_monitor.cpp
    )
    if(ANDROID)
        target_link_libraries(test_thermal_monitor PRIVATE log m)
    else()
        target_link_libraries(test_thermal_monitor PRIVATE pthread m)
    endif()
    add_test(NAME ThermalMonitorTest COMMAND test_thermal_monitor)

    add_executable(test_input_event_source
        tests/test_input_event_source.cpp
        src/daemon/reactor.cpp
        src/kernel/frame_bridge.cpp
        src/kernel/input_event_source.cpp
    )
    if(NOT ANDROID)
        target_link_libraries(test_input_event_source PRIVATE pthread)
    endif()
    add_test(NAME NativeInputEventSourceTest COMMAND test_input_event_source)

    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_FOUND)
        add_test(
            NAME CliOneShotLifecycleContractTest
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_cli_lifecycle.py $<TARGET_FILE:coreflowd>
        )
    endif()

    find_program(POSIX_SH sh)
    if(POSIX_SH)
        add_test(
            NAME ModuleLifecycleTest
            COMMAND ${POSIX_SH} ${CMAKE_SOURCE_DIR}/tests/test_module_lifecycle.sh
        )
    endif()
endif()
