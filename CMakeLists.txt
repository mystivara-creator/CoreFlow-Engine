cmake_minimum_required(VERSION 3.22)

project(CoreFlowAutonomous VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CORE_FLOW_BUILD_TESTS "Build host-side deterministic tests" ON)
option(CORE_FLOW_ENABLE_LTO "Enable link-time optimization" ON)

# Enable LTO if supported for maximum performance and minimum binary size
if(CORE_FLOW_ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_ERROR)
    if(IPO_SUPPORTED)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
        message(STATUS "Link-time optimization (LTO) enabled")
    else()
        message(WARNING "IPO/LTO not supported: ${IPO_ERROR}")
    endif()
endif()

set(COREFLOW_ONNX_ROOT "" CACHE PATH
    "Root of the extracted ONNX Runtime Android package")

# Shared source files list for both daemon and unit tests
set(COREFLOW_SHARED_SOURCES
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

# Object library to avoid duplicate compilation for daemon and tests
add_library(coreflow_core OBJECT ${COREFLOW_SHARED_SOURCES})
target_include_directories(coreflow_core PUBLIC include)
target_compile_features(coreflow_core PRIVATE cxx_std_17)

if(ANDROID)
    add_executable(coreflowd
        src/main.cpp
    )
    target_link_libraries(coreflowd PRIVATE coreflow_core)

    if(NOT COREFLOW_ONNX_ROOT)
        message(FATAL_ERROR
            "COREFLOW_ONNX_ROOT is required for Android ONNX integration")
    endif()

    set(COREFLOW_ONNX_INCLUDE_DIR
        "${COREFLOW_ONNX_ROOT}/headers")
    set(COREFLOW_ONNX_LIBRARY
        "${COREFLOW_ONNX_ROOT}/jni/${ANDROID_ABI}/libonnxruntime.so")

    if(NOT EXISTS "${COREFLOW_ONNX_INCLUDE_DIR}/onnxruntime_cxx_api.h")
        message(FATAL_ERROR
            "ONNX Runtime headers not found under ${COREFLOW_ONNX_INCLUDE_DIR}")
    endif()

    if(NOT EXISTS "${COREFLOW_ONNX_LIBRARY}")
        message(FATAL_ERROR
            "ONNX Runtime library not found: ${COREFLOW_ONNX_LIBRARY}")
    endif()

    add_library(coreflow_onnxruntime SHARED IMPORTED)
    set_target_properties(coreflow_onnxruntime PROPERTIES
        IMPORTED_LOCATION "${COREFLOW_ONNX_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${COREFLOW_ONNX_INCLUDE_DIR}")

    target_include_directories(coreflowd PRIVATE
        "${COREFLOW_ONNX_INCLUDE_DIR}")

    target_compile_definitions(coreflowd PRIVATE
        CORE_FLOW_ANDROID=1
    )

    target_compile_options(coreflowd PRIVATE
        -O2
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wshadow
        -Wformat=2
        -Wundef
        -Werror
        -fstack-protector-strong
        -fPIE
        -ffunction-sections
        -fdata-sections
    )

    target_link_options(coreflowd PRIVATE
        -pie
        -Wl,-z,relro
        -Wl,-z,now
        -Wl,--gc-sections
    )

    find_library(log-lib log REQUIRED)
    target_link_libraries(coreflowd PRIVATE ${log-lib} coreflow_onnxruntime)
else()
    message(STATUS "Host configure: Android daemon target skipped")
endif()

include(CTest)
if(BUILD_TESTING AND CORE_FLOW_BUILD_TESTS)
    enable_testing()

    # Helper macro to build test executables cleanly without code duplication
    macro(coreflow_add_test test_name test_source)
        add_executable(${test_name} ${test_source})
        target_link_libraries(${test_name} PRIVATE coreflow_core)
        target_compile_features(${test_name} PRIVATE cxx_std_17)
        target_compile_options(${test_name} PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
        )
        add_test(NAME ${test_name} COMMAND ${test_name})
    endmacro()

    coreflow_add_test(coreflow_policy_tests tests/policy_tests.cpp)
    coreflow_add_test(coreflow_config_tests tests/config_tests.cpp)
    coreflow_add_test(coreflow_baseline_tests tests/baseline_intelligence_tests.cpp)
    coreflow_add_test(coreflow_mutation_tests tests/mutation_tests.cpp)
endif()
