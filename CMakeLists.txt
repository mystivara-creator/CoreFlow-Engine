cmake_minimum_required(VERSION 3.22.1)
project(CoreFlowEngine VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CORE_FLOW_BUILD_TESTS "Build CoreFlow unit test suite" ON)

# Optimization & Security Hardening Flags for Native Systems
add_compile_options(
    -O3
    -flto
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
    -flto
    -Wl,--gc-sections
    -Wl,--strip-all
    -Wl,-z,relro
    -Wl,-z,now
)

include_directories(include)

# Modern CoreFlow Kernel & System Sources
set(COREFLOW_KERNEL_SOURCES
    src/kernel/uclamp.cpp
    src/kernel/baseline_store.cpp
    src/kernel/device_profiler.cpp
)

# Main Daemon (coreflowd)
add_executable(coreflowd
    src/daemon/main.cpp
    src/daemon/reactor.cpp
    ${COREFLOW_KERNEL_SOURCES}
)

if(ANDROID)
    target_link_libraries(coreflowd PRIVATE log)
endif()

# Unit Test Target
if(CORE_FLOW_BUILD_TESTS)
    enable_testing()
    add_executable(test_baseline_rollback
        tests/test_baseline_rollback.cpp
        src/kernel/baseline_store.cpp
    )
    target_include_directories(test_baseline_rollback PRIVATE include)
    if(ANDROID)
        target_link_libraries(test_baseline_rollback PRIVATE log)
    endif()
    add_test(NAME BaselineRollbackTest COMMAND test_baseline_rollback)
    add_executable(test_config tests/test_config.cpp)
    target_include_directories(test_config PRIVATE include)
    add_test(NAME ConfigValidationTest COMMAND test_config)
    add_executable(test_uclamp tests/test_uclamp.cpp src/kernel/uclamp.cpp src/kernel/baseline_store.cpp)
    target_include_directories(test_uclamp PRIVATE include)
    add_test(NAME UclampCgroupPercentageTest COMMAND test_uclamp)
    add_executable(test_stability_policy tests/test_stability_policy.cpp)
    target_include_directories(test_stability_policy PRIVATE include)
    add_test(NAME StabilityPolicyTest COMMAND test_stability_policy)
endif()
