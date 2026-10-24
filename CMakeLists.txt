cmake_minimum_required(VERSION 3.22)

project(CoreFlowEngine VERSION 3.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(COREFLOW_ENABLE_ASAN_UBSAN "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(COREFLOW_ENABLE_TSAN "Enable ThreadSanitizer" OFF)

if(COREFLOW_ENABLE_ASAN_UBSAN AND COREFLOW_ENABLE_TSAN)
    message(FATAL_ERROR "ASan/UBSan and TSan cannot be enabled together.")
endif()

# Validate sanitizer compile AND link support before configuring targets.
set(COREFLOW_SANITIZER_FLAGS "")
if(COREFLOW_ENABLE_ASAN_UBSAN OR COREFLOW_ENABLE_TSAN)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
        message(FATAL_ERROR "Requested sanitizers require a GNU- or Clang-compatible compiler.")
    endif()

    include(CheckCXXSourceCompiles)

    if(COREFLOW_ENABLE_ASAN_UBSAN)
        set(_coreflow_sanitizer "-fsanitize=address,undefined")
        set(_coreflow_check COREFLOW_ASAN_UBSAN_LINKS)
    else()
        set(_coreflow_sanitizer "-fsanitize=thread")
        set(_coreflow_check COREFLOW_TSAN_LINKS)
    endif()

    set(_coreflow_saved_required_flags "${CMAKE_REQUIRED_FLAGS}")
    set(_coreflow_saved_required_link_options "${CMAKE_REQUIRED_LINK_OPTIONS}")

    string(APPEND CMAKE_REQUIRED_FLAGS " ${_coreflow_sanitizer} -fno-omit-frame-pointer")
    set(CMAKE_REQUIRED_LINK_OPTIONS "${_coreflow_sanitizer}")

    check_cxx_source_compiles(
        "int main() { return 0; }"
        ${_coreflow_check}
    )

    set(CMAKE_REQUIRED_FLAGS "${_coreflow_saved_required_flags}")
    set(CMAKE_REQUIRED_LINK_OPTIONS "${_coreflow_saved_required_link_options}")

    if(NOT ${_coreflow_check})
        message(FATAL_ERROR
            "Sanitizer ${_coreflow_sanitizer} failed the compile/link check. "
            "Inspect CMakeFiles/CMakeConfigureLog.yaml for the linker diagnostic."
        )
    endif()

    list(APPEND COREFLOW_SANITIZER_FLAGS
        "${_coreflow_sanitizer}"
        "-fno-omit-frame-pointer"
    )
endif()

# Shared compiler and sanitizer settings for each executable.
function(coreflow_configure_target target_name)
    target_include_directories(${target_name} PRIVATE
        "${PROJECT_SOURCE_DIR}/include"
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
        target_compile_options(${target_name} PRIVATE
            -Wall -Wextra -Wpedantic
        )
    elseif(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /permissive-)
    endif()

    if(COREFLOW_SANITIZER_FLAGS)
        target_compile_options(${target_name} PRIVATE ${COREFLOW_SANITIZER_FLAGS})
        target_link_options(${target_name} PRIVATE ${COREFLOW_SANITIZER_FLAGS})
    endif()
endfunction()

# Production executable. CONFIGURE_DEPENDS refreshes the source list when files change.
file(GLOB_RECURSE COREFLOW_SOURCES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

if(NOT COREFLOW_SOURCES)
    message(FATAL_ERROR "No production C++ sources found under src/.")
endif()

add_executable(daemon ${COREFLOW_SOURCES})
coreflow_configure_target(daemon)

if(ANDROID)
    target_link_libraries(daemon PRIVATE log)
endif()

include(CTest)

function(coreflow_add_test target_name)
    add_executable(${target_name} ${ARGN})
    coreflow_configure_target(${target_name})

    # Keep assert-based tests active in Release builds.
    if(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
        target_compile_options(${target_name} PRIVATE -UNDEBUG)
    elseif(MSVC)
        target_compile_options(${target_name} PRIVATE /UNDEBUG)
    endif()

    add_test(NAME ${target_name} COMMAND ${target_name})
endfunction()

if(BUILD_TESTING)
    coreflow_add_test(strategy_catalog_tests
        tests/strategy_catalog_tests.cpp
        src/discovery/strategy_catalog.cpp
    )

    coreflow_add_test(architecture_contract_tests
        tests/architecture_contract_tests.cpp
    )

    coreflow_add_test(optimization_feedback_tests
        tests/optimization_feedback_tests.cpp
        src/optimization/optimization.cpp
        src/optimization/workload.cpp
    )

    coreflow_add_test(mutation_safety_tests
        tests/mutation_safety_tests.cpp
        src/mutation/mutation.cpp
        src/validation/validation.cpp
    )

    coreflow_add_test(device_profile_tests
        tests/device_profile_tests.cpp
        src/profile/device_profile.cpp
    )

    coreflow_add_test(outcome_feedback_tests
        tests/outcome_feedback_tests.cpp
        src/outcome/outcome.cpp
    )

    coreflow_add_test(workload_policy_tests
        tests/workload_policy_tests.cpp
        src/optimization/workload.cpp
    )

    coreflow_add_test(pressure_tests
        tests/pressure_tests.cpp
    )

    coreflow_add_test(surface_flinger_tests
        tests/surface_flinger_tests.cpp
        src/observation/surface_flinger.cpp
    )

    coreflow_add_test(thermal_policy_tests
        tests/thermal_policy_tests.cpp
    )

    coreflow_add_test(rollback_guard_tests
        tests/rollback_guard_tests.cpp
    )

    coreflow_add_test(discovery_classification_tests
        tests/discovery_classification_tests.cpp
        src/discovery/classification.cpp
    )

    coreflow_add_test(runtime_state_tests
        tests/runtime_state_tests.cpp
        src/runtime/state_store.cpp
    )

    coreflow_add_test(thermal_headroom_tests
        tests/thermal_headroom_tests.cpp
    )

    coreflow_add_test(optimization_coordination_tests
        tests/optimization_coordination_tests.cpp
        src/optimization/optimization.cpp
        src/optimization/workload.cpp
        src/outcome/outcome.cpp
        src/discovery/strategy_catalog.cpp
    )
    # The coordination test validates the shipped catalogue in the source tree.
    target_compile_definitions(optimization_coordination_tests PRIVATE
        COREFLOW_SOURCE_DIR="${PROJECT_SOURCE_DIR}")

    message(STATUS "CoreFlow tests: enabled")
else()
    message(STATUS "CoreFlow tests: disabled")
endif()

if(COREFLOW_ENABLE_ASAN_UBSAN)
    message(STATUS "CoreFlow sanitizers: ASan + UBSan")
elseif(COREFLOW_ENABLE_TSAN)
    message(STATUS "CoreFlow sanitizers: TSan")
else()
    message(STATUS "CoreFlow sanitizers: disabled")

endif()
