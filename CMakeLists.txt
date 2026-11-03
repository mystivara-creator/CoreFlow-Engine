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
    src/kernel/cgroup_v2.cpp
    src/kernel/uclamp.cpp
    src/kernel/baseline_store.cpp
    src/kernel/render_booster.cpp
    src/kernel/device_profiler.cpp
    src/kernel/psi_monitor.cpp
    src/kernel/memory_guardian.cpp
    src/kernel/hardware_longevity.cpp
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
endif()
