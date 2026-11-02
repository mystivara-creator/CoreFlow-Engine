cmake_minimum_required(VERSION 3.22.1)
project(CoreFlowEngine VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimization Flags for Android Native Daemons
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
)

add_link_options(
    -flto
    -Wl,--gc-sections
    -Wl,--strip-all
)

include_directories(include)

add_executable(coreflowd
    src/daemon/main.cpp
    src/daemon/reactor.cpp
    src/kernel/cgroup_v2.cpp
    src/kernel/uclamp.cpp
    src/kernel/baseline_store.cpp
    src/kernel/render_booster.cpp
    src/kernel/device_profiler.cpp
)

target_link_libraries(coreflowd PRIVATE
    log
)
