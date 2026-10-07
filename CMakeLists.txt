################################################################################
# CMakeLists.txt - CoreFlowAutonomous
# 
# Modern CMake 3.22+ configuration with best practices:
# - Target-based approach (no global properties)
# - Feature detection and version management
# - Proper error handling and validation
# - Support for multiple build modes (Android/Host)
# - Export targets for library usage
# - Comprehensive documentation
################################################################################

cmake_minimum_required(VERSION 3.22)

project(
    CoreFlowAutonomous
    VERSION 1.1.0
    DESCRIPTION "Intelligent autonomous flow control system"
    HOMEPAGE_URL "https://example.com/coreflow"
    LANGUAGES CXX
)

# ============================================================================
# Project Settings & Policies
# ============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Modern CMake policies for consistency
cmake_policy(SET CMP0074 NEW)  # find_package uses <PackageName>_ROOT
cmake_policy(SET CMP0077 NEW)  # option() honors normal variables

# Build output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# ============================================================================
# Project Options
# ============================================================================
option(CORE_FLOW_BUILD_TESTS 
    "Build host-side deterministic tests" ON)
option(CORE_FLOW_BUILD_DOCS 
    "Build documentation (requires Doxygen)" OFF)
option(CORE_FLOW_ENABLE_LTO 
    "Enable link-time optimization" ON)
option(CORE_FLOW_SANITIZERS 
    "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

# ============================================================================
# Version & Git Information
# ============================================================================
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.cmake)
coreflow_version_info()

message(STATUS "Building ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Git commit: ${COREFLOW_GIT_COMMIT_SHORT}")

# ============================================================================
# Compiler & Linker Configuration
# ============================================================================
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/compiler_flags.cmake)
coreflow_set_compiler_flags()

if(CORE_FLOW_ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT IPO_SUPPORTED)
    if(IPO_SUPPORTED)
        message(STATUS "Link-time optimization enabled")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(WARNING "IPO not supported by compiler")
    endif()
endif()

if(CORE_FLOW_SANITIZERS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address -fsanitize=undefined")
    message(STATUS "Sanitizers enabled (AddressSanitizer + UndefinedBehaviorSanitizer)")
endif()

# ============================================================================
# Dependencies
# ============================================================================

# Android-specific: ONNX Runtime
if(ANDROID)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/onnx_android.cmake)
    coreflow_find_onnx_runtime()
endif()

# ============================================================================
# Core Library (shared components)
# ============================================================================
add_library(coreflow_core OBJECT
    src/config.cpp
    src/signal_state.cpp
    src/autonomous.cpp
    src/thermal_predictor.cpp
    src/baseline_intelligence.cpp
    src/discovery.cpp
    src/observer.cpp
    src/policy.cpp
    src/mutation.cpp
)

set_target_properties(coreflow_core PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

target_include_directories(coreflow_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# ============================================================================
# Android Daemon Target
# ============================================================================
if(ANDROID)
    add_executable(coreflowd src/main.cpp)
    
    set_target_properties(coreflowd PROPERTIES
        VERSION ${PROJECT_VERSION}
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
    )
    
    target_link_libraries(coreflowd PRIVATE
        coreflow_core
        log
        coreflow_onnxruntime
    )
    
    target_compile_definitions(coreflowd PRIVATE
        CORE_FLOW_VERSION="${PROJECT_VERSION}"
        CORE_FLOW_ANDROID=1
        CORE_FLOW_GIT_COMMIT="${COREFLOW_GIT_COMMIT_SHORT}"
    )
    
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/android_hardening.cmake)
    coreflow_apply_android_hardening(coreflowd)
    
    # Install target
    install(TARGETS coreflowd
        RUNTIME DESTINATION bin
        BUNDLE DESTINATION .
    )
    
    message(STATUS "✓ Android daemon (coreflowd) configured")
else()
    message(STATUS "⊘ Android daemon target skipped (not Android platform)")
endif()

# ============================================================================
# Tests
# ============================================================================
include(CTest)

if(BUILD_TESTING AND CORE_FLOW_BUILD_TESTS)
    enable_testing()
    message(STATUS "Test suite enabled")
    add_subdirectory(tests)
endif()

# ============================================================================
# Installation & Export
# ============================================================================
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/install.cmake)
coreflow_install_targets()

# ============================================================================
# Summary
# ============================================================================
message(STATUS "
================================================================================
  ${PROJECT_NAME} v${PROJECT_VERSION} Configuration Summary
================================================================================
  Platform:              ${CMAKE_SYSTEM_NAME}
  Compiler:              ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}
  Build Type:            ${CMAKE_BUILD_TYPE}
  C++ Standard:          C++${CMAKE_CXX_STANDARD}
  
  Features:
    Tests:               ${CORE_FLOW_BUILD_TESTS}
    LTO:                 ${CORE_FLOW_ENABLE_LTO}
    Sanitizers:          ${CORE_FLOW_SANITIZERS}
    Docs:                ${CORE_FLOW_BUILD_DOCS}
  
  Directories:
    Source:              ${CMAKE_CURRENT_SOURCE_DIR}
    Binary:              ${CMAKE_CURRENT_BINARY_DIR}
    Install:             ${CMAKE_INSTALL_PREFIX}
================================================================================
")
