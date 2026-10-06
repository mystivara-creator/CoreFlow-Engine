cmake_minimum_required(VERSION 3.22)

project(CoreFlowAutonomous VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CORE_FLOW_BUILD_TESTS "Build host-side deterministic tests" ON)

set(COREFLOW_ONNX_ROOT "" CACHE PATH
    "Root of the extracted ONNX Runtime Android package")
if(ANDROID)
    add_executable(coreflowd
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
        include
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
    add_executable(coreflow_policy_tests
        tests/policy_tests.cpp
        src/policy.cpp
    )
    add_executable(coreflow_config_tests
        tests/config_tests.cpp
        src/config.cpp
    )
    add_executable(coreflow_baseline_tests
        tests/baseline_intelligence_tests.cpp
        src/baseline_intelligence.cpp
    )
    add_executable(coreflow_mutation_tests
        tests/mutation_tests.cpp
        src/mutation.cpp
        src/config.cpp
    )
    target_include_directories(coreflow_policy_tests PRIVATE include)
    target_include_directories(coreflow_config_tests PRIVATE include)
    target_include_directories(coreflow_baseline_tests PRIVATE include)
    target_include_directories(coreflow_mutation_tests PRIVATE include)
    target_compile_features(coreflow_policy_tests PRIVATE cxx_std_17)
    target_compile_features(coreflow_config_tests PRIVATE cxx_std_17)
    target_compile_features(coreflow_baseline_tests PRIVATE cxx_std_17)
    target_compile_features(coreflow_mutation_tests PRIVATE cxx_std_17)
    target_compile_options(coreflow_policy_tests PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
    )
    target_compile_options(coreflow_config_tests PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
    )
    target_compile_options(coreflow_baseline_tests PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
    )
    target_compile_options(coreflow_mutation_tests PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror
    )
    add_test(NAME coreflow_policy_tests COMMAND coreflow_policy_tests)
    add_test(NAME coreflow_config_tests COMMAND coreflow_config_tests)
    add_test(NAME coreflow_baseline_tests COMMAND coreflow_baseline_tests)
    add_test(NAME coreflow_mutation_tests COMMAND coreflow_mutation_tests)
endif()
