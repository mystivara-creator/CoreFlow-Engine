cmake_minimum_required(VERSION 3.23)

project(CoreFlowAutonomous VERSION 2.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(COREFLOW_ONNX_ROOT "" CACHE PATH
    "Root of the extracted ONNX Runtime Android package")
option(CORE_FLOW_BUILD_TESTS "Build host deterministic tests" OFF)

add_library(coreflow_compile_options INTERFACE)

target_compile_features(coreflow_compile_options INTERFACE cxx_std_20)

target_compile_options(coreflow_compile_options
    INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wshadow
        -Wformat=2
        -Wundef
)

# Platform-independent logic. Compiled for both the Android daemon and the
# host test binary. Anything that needs Android/ONNX headers stays Android-only.
# Source layout mirrors the engine pillars (not a grab-bag of files):
#   observe/    — discovery & sampling
#   understand/ — context, effect model, experience, thermal intelligence
#   balance/    — policy gates & safety hold
#   adapt/      — mutation controllers, actuators, outcome measurement
#   conscience/ — journals, retained state, decision trace
#   runtime/    — daemon entry, tick loop, config, status export
set(COREFLOW_CORE_SOURCES
    src/observe/environment.cpp
    src/observe/ecosystem_knowledge.cpp
    src/observe/high_capability.cpp
    src/understand/context.cpp
    src/understand/effect_model.cpp
    src/understand/decision_agent.cpp
    src/understand/experience.cpp
    src/understand/thermal_features.cpp
    src/understand/thermal_guard.cpp
    src/balance/policy.cpp
    src/balance/control.cpp
    src/balance/safety_hold.cpp
    src/adapt/resource_model.cpp
    src/adapt/resource_actuator.cpp
    src/adapt/resource_mutation.cpp
    src/adapt/cpufreq_actuator.cpp
    src/adapt/mutation.cpp
    src/adapt/baseline_intelligence.cpp
    src/adapt/outcome.cpp
    src/conscience/mutation_journal.cpp
    src/conscience/retained_state.cpp
    src/runtime/config.cpp
    src/runtime/signal_state.cpp
    src/runtime/status_snapshot.cpp
    src/runtime/runtime_paths.cpp
    src/balance/capability_registry.cpp
)

set(COREFLOW_ANDROID_SOURCES
    src/runtime/main.cpp
    src/runtime/autonomous.cpp
    src/understand/thermal_predictor.cpp
    src/conscience/decision_trace.cpp
    src/observe/discovery.cpp
    src/observe/observer.cpp
)

if(ANDROID)

    message(STATUS "Configuring CoreFlow Autonomous Engine for Android")
    message(STATUS "CoreFlow version: ${PROJECT_VERSION}")
    message(STATUS "Android ABI: ${ANDROID_ABI}")
    message(STATUS "Android platform: ${ANDROID_PLATFORM}")

    if(NOT COREFLOW_ONNX_ROOT)
        message(FATAL_ERROR
            "COREFLOW_ONNX_ROOT is required for Android ONNX integration.")
    endif()

    set(COREFLOW_ONNX_INCLUDE_DIR "${COREFLOW_ONNX_ROOT}/headers")
    set(COREFLOW_ONNX_LIBRARY "${COREFLOW_ONNX_ROOT}/jni/${ANDROID_ABI}/libonnxruntime.so")

    if(NOT EXISTS "${COREFLOW_ONNX_INCLUDE_DIR}/onnxruntime_cxx_api.h")
        message(FATAL_ERROR
            "ONNX Runtime headers not found:\n  ${COREFLOW_ONNX_INCLUDE_DIR}/onnxruntime_cxx_api.h")
    endif()

    if(NOT EXISTS "${COREFLOW_ONNX_LIBRARY}")
        message(FATAL_ERROR
            "ONNX Runtime library not found:\n  ${COREFLOW_ONNX_LIBRARY}")
    endif()

    add_library(coreflow_onnxruntime SHARED IMPORTED GLOBAL)
    set_target_properties(coreflow_onnxruntime PROPERTIES
        IMPORTED_LOCATION "${COREFLOW_ONNX_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${COREFLOW_ONNX_INCLUDE_DIR}"
    )

    add_executable(coreflowd
        ${COREFLOW_CORE_SOURCES}
        ${COREFLOW_ANDROID_SOURCES}
    )

    target_include_directories(coreflowd PRIVATE "${PROJECT_SOURCE_DIR}/include")

    target_link_libraries(coreflowd
        PRIVATE
            coreflow_compile_options
            coreflow_onnxruntime
    )

    target_compile_definitions(coreflowd PRIVATE CORE_FLOW_ANDROID=1)

    target_compile_options(coreflowd
        PRIVATE
            -O2
            -fstack-protector-strong
            -ffunction-sections
            -fdata-sections
            -Werror
    )

    target_link_options(coreflowd
        PRIVATE
            -Wl,-z,relro
            -Wl,-z,now
            -Wl,--gc-sections
    )

    find_library(ANDROID_LOG_LIBRARY log REQUIRED)
    target_link_libraries(coreflowd PRIVATE ${ANDROID_LOG_LIBRARY})

    message(STATUS "CoreFlow target: coreflowd")
    message(STATUS "ONNX Runtime root: ${COREFLOW_ONNX_ROOT}")

else()

    message(STATUS "Host configure: Android daemon target skipped.")

    if(CORE_FLOW_BUILD_TESTS)
        enable_testing()

        add_library(coreflow_core STATIC ${COREFLOW_CORE_SOURCES})
        target_include_directories(coreflow_core PUBLIC "${PROJECT_SOURCE_DIR}/include")
        target_link_libraries(coreflow_core PUBLIC coreflow_compile_options)
        target_compile_definitions(coreflow_core PRIVATE COREFLOW_HOST_TEST_FIXTURES=1)
        target_compile_options(coreflow_core PRIVATE -Werror)

        add_executable(coreflow_tests tests/mutation_tests.cpp)
        target_link_libraries(coreflow_tests PRIVATE coreflow_core)
        target_compile_options(coreflow_tests PRIVATE -Werror)

        add_executable(coreflow_architecture_tests tests/architecture_tests.cpp)
        target_link_libraries(coreflow_architecture_tests PRIVATE coreflow_core)
        target_compile_options(coreflow_architecture_tests PRIVATE -Werror)

        add_executable(coreflow_resource_tests tests/resource_tests.cpp)
        target_link_libraries(coreflow_resource_tests PRIVATE coreflow_core)
        target_compile_options(coreflow_resource_tests PRIVATE -Werror)

        add_test(NAME coreflow_mutation_safety COMMAND coreflow_tests)
        add_test(NAME coreflow_architecture_foundation COMMAND coreflow_architecture_tests)
        add_executable(coreflow_capability_expansion_tests tests/high_capability_tests.cpp)
        target_link_libraries(coreflow_capability_expansion_tests PRIVATE coreflow_core)
        target_compile_options(coreflow_capability_expansion_tests PRIVATE -Werror)
        add_test(NAME coreflow_high_capability_expansion COMMAND coreflow_capability_expansion_tests)
        add_test(NAME coreflow_resource_autonomy COMMAND coreflow_resource_tests)
        add_executable(coreflow_release_hardening_tests tests/release_hardening_tests.cpp)
        target_link_libraries(coreflow_release_hardening_tests PRIVATE coreflow_core)
        target_compile_options(coreflow_release_hardening_tests PRIVATE -Werror)
        target_compile_definitions(coreflow_release_hardening_tests
            PRIVATE COREFLOW_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\")

        add_test(NAME coreflow_release_hardening COMMAND coreflow_release_hardening_tests)
        add_test(NAME coreflow_safety_foundation_contract
            COMMAND bash "${PROJECT_SOURCE_DIR}/tools/verify_safety_foundation.sh")
        add_test(NAME coreflow_source_release_contract
            COMMAND "${CMAKE_COMMAND}" -E env COREFLOW_ALLOW_BUILD_DIR=1 bash
                    "${PROJECT_SOURCE_DIR}/tools/verify_source_release.sh")
    endif()

endif()
