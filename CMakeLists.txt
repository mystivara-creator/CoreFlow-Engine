cmake_minimum_required(VERSION 3.10)
project(CoreFlowEngine CXX)

# Gunakan standar C++17 agar kompatibel dengan std::this_thread dan file stream modern
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Tambahkan folder 'include' agar compiler bisa menemukan header file coreflow.hpp
include_directories(include)

# Kumpulkan semua file C++ secara otomatis di dalam folder 'src' (Mencegah duplikasi manual)
file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")

# Perintahkan untuk membuat binary executable bernama 'coreflow_daemon'
add_executable(coreflow_daemon ${SOURCE_FILES})

# Tautkan pustaka Android Logcat (-llog) dan Threading bawaan Linux NDK
target_link_libraries(coreflow_daemon log pthread)
