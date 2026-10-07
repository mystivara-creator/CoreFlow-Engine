cmake_minimum_required(VERSION 3.23)

project(CoreFlowAutonomous
    VERSION 1.1.0
    LANGUAGES CXX
)

# ============================================================
# Project Configuration
# ============================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CORE_FLOW_BUILD_TESTS
    "Build host-side deterministic tests"
    ON
)

set(COREFLOW_ONNX_ROOT ""
    CACHE PATH
    "Root of the extracted ONNX Runtime Android package"
)

# ============================================================
# Common Compiler Configuration
# ============================================================

add_library(coreflow_compile_options INTERFACE)

target_compile_features(coreflow_compile_options
    INTERFACE
        cxx_std_17
)

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

# ============================================================
# CoreFlow Source Definition
# ============================================================

set(COREFLOW_SOURCES
    src/main.cpp
    src/config.cpp
    src/signal_state.cpp
    src/autonomous.cpp
    src/thermal_predictor.cpp
    src/baseline_intelligence.cpp
    src/discovery.cpp
    src/observer.cpp
    src/policy.cpp
    src/mutation.cpp
)

# ============================================================
# Android Target
# ============================================================

if(ANDROID)

    message(STATUS "Configuring CoreFlow Autonomous Engine for Android")
    message(STATUS "Android ABI: ${ANDROID_ABI}")
    message(STATUS "Android platform: ${ANDROID_PLATFORM}")

    # --------------------------------------------------------
    # ONNX Runtime Validation
    # --------------------------------------------------------

    if(NOT COREFLOW_ONNX_ROOT)
        message(FATAL_ERROR
            "COREFLOW_ONNX_ROOT is required for Android ONNX integration."
        )
    endif()

    set(COREFLOW_ONNX_INCLUDE_DIR
        "${COREFLOW_ONNX_ROOT}/headers"
    )

    set(COREFLOW_ONNX_LIBRARY
        "${COREFLOW_ONNX_ROOT}/jni/${ANDROID_ABI}/libonnxruntime.so"
    )

    if(NOT EXISTS "${COREFLOW_ONNX_INCLUDE_DIR}/onnxruntime_cxx_api.h")
        message(FATAL_ERROR
            "ONNX Runtime headers not found:\n"
            "  ${COREFLOW_ONNX_INCLUDE_DIR}/onnxruntime_cxx_api.h"
        )
    endif()

    if(NOT EXISTS "${COREFLOW_ONNX_LIBRARY}")
        message(FATAL_ERROR
            "ONNX Runtime library not found:\n"
            "  ${COREFLOW_ONNX_LIBRARY}"
        )
    endif()

    # --------------------------------------------------------
    # ONNX Runtime Imported Target
    # --------------------------------------------------------

    add_library(coreflow_onnxruntime SHARED IMPORTED GLOBAL)

    set_target_properties(coreflow_onnxruntime PROPERTIES
        IMPORTED_LOCATION
            "${COREFLOW_ONNX_LIBRARY}"

        INTERFACE_INCLUDE_DIRECTORIES
            "${COREFLOW_ONNX_INCLUDE_DIR}"
    )

    # --------------------------------------------------------
    # CoreFlow Daemon
    # --------------------------------------------------------

    add_executable(coreflowd
        ${COREFLOW_SOURCES}
    )

    target_include_directories(coreflowd
        PRIVATE
            "${PROJECT_SOURCE_DIR}/include"
    )

    target_link_libraries(coreflowd
        PRIVATE
            coreflow_compile_options
            coreflow_onnxruntime
    )

    target_compile_definitions(coreflowd
        PRIVATE
            CORE_FLOW_ANDROID=1
    )

    # --------------------------------------------------------
    # Android-specific Compiler Configuration
    # --------------------------------------------------------

    target_compile_options(coreflowd
        PRIVATE
            -O2
            -fstack-protector-strong
            -ffunction-sections
            -fdata-sections
            -Werror
    )

    # --------------------------------------------------------
    # Android-specific Linker Configuration
    # --------------------------------------------------------

    target_link_options(coreflowd
        PRIVATE
            -Wl,-z,relro
            -Wl,-z,now
            -Wl,--gc-sections
    )

    # --------------------------------------------------------
    # Android Log Library
    # --------------------------------------------------------

    find_library(
        ANDROID_LOG_LIBRARY
        log
        REQUIRED
    )

    target_link_libraries(coreflowd
        PRIVATE
            ${ANDROID_LOG_LIBRARY}
    )

    # --------------------------------------------------------
    # Diagnostics
    # --------------------------------------------------------

    message(STATUS "CoreFlow target: coreflowd")
    message(STATUS "ONNX Runtime root: ${COREFLOW_ONNX_ROOT}")
    message(STATUS "ONNX Runtime ABI: ${ANDROID_ABI}")

else()

    message(STATUS
        "Host configure detected: Android daemon target skipped."
    )

endif()

# ============================================================
# Host-side Tests
# ============================================================

include(CTest)

if(BUILD_TESTING AND CORE_FLOW_BUILD_TESTS)

    message(STATUS "Configuring CoreFlow host-side tests")

    # --------------------------------------------------------
    # Common Test Options
    # --------------------------------------------------------

    add_library(coreflow_test_options INTERFACE)

    target_link_libraries(coreflow_test_options
        INTERFACE
            coreflow_compile_options
    )

    target_compile_options(coreflow_test_options
        INTERFACE
            -Werror
    )

    # --------------------------------------------------------
    # Policy Tests
    # --------------------------------------------------------

    add_executable(coreflow_policy_tests
        tests/policy_tests.cpp
        src/policy.cpp
    )

    target_include_directories(coreflow_policy_tests
        PRIVATE
            "${PROJECT_SOURCE_DIR}/include"
    )

    target_link_libraries(coreflow_policy_tests
        PRIVATE
            coreflow_test_options
    )

    add_test(
        NAME coreflow_policy_tests
        COMMAND coreflow_policy_tests
    )

    # --------------------------------------------------------
    # Config Tests
    # --------------------------------------------------------

    add_executable(coreflow_config_tests
        tests/config_tests.cpp
        src/config.cpp
    )

    target_include_directories(coreflow_config_tests
        PRIVATE
            "${PROJECT_SOURCE_DIR}/include"
    )

    target_link_libraries(coreflow_config_tests
        PRIVATE
            coreflow_test_options
    )

    add_test(
        NAME coreflow_config_tests
        COMMAND coreflow_config_tests
    )

    # --------------------------------------------------------
    # Baseline Intelligence Tests
    # --------------------------------------------------------

    add_executable(coreflow_baseline_tests
        tests/baseline_intelligence_tests.cpp
        src/baseline_intelligence.cpp
    )

    target_include_directories(coreflow_baseline_tests
        PRIVATE
            "${PROJECT_SOURCE_DIR}/include"
    )

    target_link_libraries(coreflow_baseline_tests
        PRIVATE
            coreflow_test_options
    )

    add_test(
        NAME coreflow_baseline_tests
        COMMAND coreflow_baseline_tests
    )

    # --------------------------------------------------------
    # Mutation Tests
    # --------------------------------------------------------

    add_executable(coreflow_mutation_tests
        tests/mutation_tests.cpp
        src/mutation.cpp
        src/config.cpp
    )

    target_include_directories(coreflow_mutation_tests
        PRIVATE
            "${PROJECT_SOURCE_DIR}/include"
    )

    target_link_libraries(coreflow_mutation_tests
        PRIVATE
            coreflow_test_options
    )

    add_test(
        NAME coreflow_mutation_tests
        COMMAND coreflow_mutation_tests
    )

endif()