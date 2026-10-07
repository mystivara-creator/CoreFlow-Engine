################################################################################
# cmake/onnx_android.cmake
#
# ONNX Runtime Android detection and configuration
# Validates paths, sets up imported library target, and provides diagnostics
################################################################################

function(coreflow_find_onnx_runtime)
    message(STATUS "Detecting ONNX Runtime for Android...")
    
    # Get ONNX root from CMake cache or environment
    if(NOT COREFLOW_ONNX_ROOT)
        set(COREFLOW_ONNX_ROOT "$ENV{COREFLOW_ONNX_ROOT}" CACHE PATH
            "Root directory of extracted ONNX Runtime Android package" FORCE)
    endif()
    
    if(NOT COREFLOW_ONNX_ROOT)
        message(FATAL_ERROR
            "COREFLOW_ONNX_ROOT not set. Please provide the path to ONNX Runtime:\n"
            "  cmake -DCOREFLOW_ONNX_ROOT=/path/to/onnxruntime-android-<version>\n"
            "Or set environment variable: export COREFLOW_ONNX_ROOT=..."
        )
    endif()
    
    # Validate ONNX_ROOT directory exists
    if(NOT EXISTS "${COREFLOW_ONNX_ROOT}")
        message(FATAL_ERROR 
            "COREFLOW_ONNX_ROOT does not exist: ${COREFLOW_ONNX_ROOT}")
    endif()
    
    message(STATUS "  ONNX Root: ${COREFLOW_ONNX_ROOT}")
    message(STATUS "  Android ABI: ${ANDROID_ABI}")
    
    # Construct expected paths
    set(ONNX_INCLUDE_DIR "${COREFLOW_ONNX_ROOT}/headers")
    set(ONNX_LIBRARY_DIR "${COREFLOW_ONNX_ROOT}/jni/${ANDROID_ABI}")
    set(ONNX_LIBRARY_PATH "${ONNX_LIBRARY_DIR}/libonnxruntime.so")
    
    # Validate headers
    set(ONNX_HEADER_CHECK "${ONNX_INCLUDE_DIR}/onnxruntime_cxx_api.h")
    if(NOT EXISTS "${ONNX_HEADER_CHECK}")
        message(FATAL_ERROR
            "ONNX Runtime headers not found at: ${ONNX_INCLUDE_DIR}\n"
            "Expected to find: ${ONNX_HEADER_CHECK}\n"
            "\nDirectory contents:\n"
            "  ${COREFLOW_ONNX_ROOT}/")
    endif()
    
    # Validate library for current ABI
    if(NOT EXISTS "${ONNX_LIBRARY_PATH}")
        message(FATAL_ERROR
            "ONNX Runtime library not found for ABI ${ANDROID_ABI}:\n"
            "  Expected: ${ONNX_LIBRARY_PATH}\n"
            "\nAvailable ABIs in ${ONNX_LIBRARY_DIR}:")
        
        # Try to list available ABIs for diagnostics
        file(GLOB AVAILABLE_ABIS "${COREFLOW_ONNX_ROOT}/jni/*/")
        foreach(ABI_PATH ${AVAILABLE_ABIS})
            get_filename_component(ABI_NAME ${ABI_PATH} NAME)
            message(FATAL_ERROR "  - ${ABI_NAME}")
        endforeach()
    endif()
    
    # Validate Version compatibility
    if(EXISTS "${ONNX_INCLUDE_DIR}/onnxruntime_config.h")
        file(STRINGS "${ONNX_INCLUDE_DIR}/onnxruntime_config.h" 
            ONNX_VERSION_STRING REGEX "ONNXRUNTIME_VERSION")
        message(STATUS "  ${ONNX_VERSION_STRING}")
    endif()
    
    # Create imported library target
    if(NOT TARGET coreflow_onnxruntime)
        add_library(coreflow_onnxruntime SHARED IMPORTED GLOBAL)
        
        set_target_properties(coreflow_onnxruntime PROPERTIES
            IMPORTED_LOCATION "${ONNX_LIBRARY_PATH}"
            INTERFACE_INCLUDE_DIRECTORIES "${ONNX_INCLUDE_DIR}"
            IMPORTED_NO_SONAME TRUE  # Important for Android cross-compilation
        )
        
        message(STATUS "✓ ONNX Runtime imported library configured")
        message(STATUS "  Include: ${ONNX_INCLUDE_DIR}")
        message(STATUS "  Library: ${ONNX_LIBRARY_PATH}")
    endif()
    
    # Make available to parent scope
    set(COREFLOW_ONNX_FOUND TRUE PARENT_SCOPE)
    set(COREFLOW_ONNX_INCLUDE_DIR "${ONNX_INCLUDE_DIR}" PARENT_SCOPE)
    set(COREFLOW_ONNX_LIBRARY_PATH "${ONNX_LIBRARY_PATH}" PARENT_SCOPE)
    
endfunction()
