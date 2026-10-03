cmake_minimum_required(VERSION 3.22)

project(CoreFlowAutonomous VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT ANDROID)
    message(FATAL_ERROR "CoreFlow Autonomous requires the Android NDK.")
endif()

add_executable(coreflowd
    src/main.cpp
    src/autonomous.cpp
    src/discovery.cpp
    src/observer.cpp
    src/policy.cpp
)

target_include_directories(coreflowd PRIVATE include)

target_compile_definitions(coreflowd PRIVATE
    CORE_FLOW_VERSION="1.0.0-A"
    CORE_FLOW_ANDROID=1
)

target_compile_options(coreflowd PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wsign-conversion
    -Wno-unused-parameter
)

find_library(log-lib log)
if(log-lib)
    target_link_libraries(coreflowd PRIVATE ${log-lib})
endif()

set_target_properties(coreflowd PROPERTIES
    OUTPUT_NAME coreflowd
)
