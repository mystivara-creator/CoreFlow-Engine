cmake_minimum_required(VERSION 3.10)
project(CoreFlowEngine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")

add_executable(coreflow_daemon ${SOURCE_FILES})

# Android NDK sudah menyediakan threading support via libc.
# Hindari find_package(Threads) dan Threads::Threads yang bisa gagal.
target_link_libraries(coreflow_daemon log)