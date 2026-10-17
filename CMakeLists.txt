cmake_minimum_required(VERSION 3.23)

project(CoreFlowAutonomous VERSION 2.4.0 LANGUAGES CXX)

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
        -Wundef)

# Sources are discovered, not listed. See cmake/CoreFlowSources.cmake for the
# platform rule. Layout follows the engine's role model:
#   observe/     eyes      environment sensing, discovery, capability registry
#   understand/  reasoning context, effect model, decision agent, thermal, baseline
#   balance/     brain     policy, safety hold, consultation, resource model
#   mutation/    hands     actuators and mutation controllers (permit required)
#   conscience/  memory    journal, retained state, decision trace, fatal guard
#   runtime/     lifecycle configuration, paths, status snapshot, daemon entry
#   autonomous/  loop      autonomous tick orchestration
include("${PROJECT_SOURCE_DIR}/cmake/CoreFlowSources.cmake")

if(ANDROID AND NOT CORE_FLOW_BUILD_TESTS)
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
        INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${COREFLOW_ONNX_INCLUDE_DIR}")

    add_executable(coreflowd
        ${COREFLOW_CORE_SOURCES}
        ${COREFLOW_ANDROID_SOURCES})

    target_include_directories(coreflowd PRIVATE "${PROJECT_SOURCE_DIR}/include")
    target_link_libraries(coreflowd
        PRIVATE
            coreflow_compile_options
            coreflow_onnxruntime)
    target_compile_definitions(coreflowd PRIVATE CORE_FLOW_ANDROID=1)
    target_compile_options(coreflowd
        PRIVATE
            -O2
            -fstack-protector-strong
            -ffunction-sections
            -fdata-sections
            -Werror)
    target_link_options(coreflowd
        PRIVATE
            -Wl,-z,relro
            -Wl,-z,now
            -Wl,--gc-sections)

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

        # Every tests/*_tests.cpp is one executable and one CTest entry, named
        # after its file. Each test file carries its own main().
        file(GLOB COREFLOW_TEST_SOURCES CONFIGURE_DEPENDS
             "${PROJECT_SOURCE_DIR}/tests/*_tests.cpp")
        foreach(_test_src IN LISTS COREFLOW_TEST_SOURCES)
            get_filename_component(_stem "${_test_src}" NAME_WE)
            set(_target "coreflow_${_stem}")
            add_executable(${_target} "${_test_src}")
            target_link_libraries(${_target} PRIVATE coreflow_core)
            target_compile_definitions(${_target}
                PRIVATE
                    COREFLOW_HOST_TEST_FIXTURES=1
                    COREFLOW_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
            target_compile_options(${_target} PRIVATE -Werror)
            add_test(NAME ${_target} COMMAND ${_target})
        endforeach()

        add_test(NAME coreflow_safety_foundation_contract
            COMMAND bash "${PROJECT_SOURCE_DIR}/tools/verify_safety_foundation.sh")
        add_test(NAME coreflow_source_release_contract
            COMMAND "${CMAKE_COMMAND}" -E env COREFLOW_ALLOW_BUILD_DIR=1
                    COREFLOW_ALLOW_IGNORED_ORT_CACHE=1 bash
                    "${PROJECT_SOURCE_DIR}/tools/verify_source_release.sh")
    endif()
endif()
