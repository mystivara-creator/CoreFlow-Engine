cmake_minimum_required(VERSION 3.10)
project(CoreFlowEngine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")

add_executable(coreflow_daemon ${SOURCE_FILES})

find_package(Threads REQUIRED)

target_link_libraries(coreflow_daemon log Threads::Threads)

# Production compile flags when building Release
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(coreflow_daemon PRIVATE
        -O3
        -flto
        -fomit-frame-pointer
        -Wall
        -Wextra
        -Wno-unused-parameter
    )
    target_link_options(coreflow_daemon PRIVATE -flto)
endif()

# Ensure Android static C++ runtime is preferred when set via CI:
#   -DANDROID_STL=c++_static