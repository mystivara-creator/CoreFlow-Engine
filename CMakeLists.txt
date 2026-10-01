cmake_minimum_required(VERSION 3.10)
project(CoreFlowEngine CXX)

# Gunakan standar C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Tambahkan folder 'include' agar compiler bisa menemukan header file
include_directories(include)

# Kumpulkan semua file C++ di dalam folder 'src'
file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")

# Perintahkan untuk membuat binary executable bernama 'coreflow_daemon'
add_executable(coreflow_daemon ${SOURCE_FILES})

# Link library Android native (log untuk logcat bawaan sistem jika diperlukan)
target_link_libraries(coreflow_daemon log)
