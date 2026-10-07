################################################################################
# cmake/version.cmake
#
# Extract version information from git and CMake project metadata
# Generates version.hpp header for runtime access
################################################################################

function(coreflow_version_info)
    # Get git information
    execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE COREFLOW_GIT_COMMIT_SHORT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    
    execute_process(
        COMMAND git rev-parse HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE COREFLOW_GIT_COMMIT_LONG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    
    execute_process(
        COMMAND git describe --tags --always
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE COREFLOW_GIT_DESCRIBE
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    
    execute_process(
        COMMAND git rev-list --count HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE COREFLOW_GIT_COMMIT_COUNT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    
    # Fallback values if git not available
    if(NOT COREFLOW_GIT_COMMIT_SHORT)
        set(COREFLOW_GIT_COMMIT_SHORT "unknown")
    endif()
    if(NOT COREFLOW_GIT_COMMIT_LONG)
        set(COREFLOW_GIT_COMMIT_LONG "unknown")
    endif()
    if(NOT COREFLOW_GIT_DESCRIBE)
        set(COREFLOW_GIT_DESCRIBE "v${PROJECT_VERSION}")
    endif()
    if(NOT COREFLOW_GIT_COMMIT_COUNT)
        set(COREFLOW_GIT_COMMIT_COUNT "0")
    endif()
    
    # Get build timestamp
    string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S" UTC)
    
    # Make available to parent scope
    set(COREFLOW_GIT_COMMIT_SHORT ${COREFLOW_GIT_COMMIT_SHORT} PARENT_SCOPE)
    set(COREFLOW_GIT_COMMIT_LONG ${COREFLOW_GIT_COMMIT_LONG} PARENT_SCOPE)
    set(COREFLOW_GIT_DESCRIBE ${COREFLOW_GIT_DESCRIBE} PARENT_SCOPE)
    set(COREFLOW_GIT_COMMIT_COUNT ${COREFLOW_GIT_COMMIT_COUNT} PARENT_SCOPE)
    set(BUILD_TIMESTAMP ${BUILD_TIMESTAMP} PARENT_SCOPE)
    
    message(STATUS "Version Information:")
    message(STATUS "  Version: ${PROJECT_VERSION}")
    message(STATUS "  Git tag: ${COREFLOW_GIT_DESCRIBE}")
    message(STATUS "  Commit: ${COREFLOW_GIT_COMMIT_SHORT} (${COREFLOW_GIT_COMMIT_COUNT} commits)")
    message(STATUS "  Built: ${BUILD_TIMESTAMP}")
    
endfunction()

################################################################################
# Generate version header file
################################################################################
function(coreflow_generate_version_header)
    set(VERSION_HEADER_IN "${CMAKE_CURRENT_SOURCE_DIR}/include/coreflow_version.hpp.in")
    set(VERSION_HEADER_OUT "${CMAKE_CURRENT_BINARY_DIR}/include/coreflow_version.hpp")
    
    if(NOT EXISTS "${VERSION_HEADER_IN}")
        # Create from scratch if template doesn't exist
        file(WRITE "${VERSION_HEADER_OUT}"
            "#pragma once\n\n"
            "#define COREFLOW_VERSION_MAJOR ${PROJECT_VERSION_MAJOR}\n"
            "#define COREFLOW_VERSION_MINOR ${PROJECT_VERSION_MINOR}\n"
            "#define COREFLOW_VERSION_PATCH ${PROJECT_VERSION_PATCH}\n"
            "#define COREFLOW_VERSION \"${PROJECT_VERSION}\"\n"
            "#define COREFLOW_GIT_COMMIT \"${COREFLOW_GIT_COMMIT_SHORT}\"\n"
            "#define COREFLOW_GIT_DESCRIBE \"${COREFLOW_GIT_DESCRIBE}\"\n"
            "#define COREFLOW_BUILD_TIMESTAMP \"${BUILD_TIMESTAMP}\"\n\n"
        )
    else()
        # Use template if available
        configure_file(
            "${VERSION_HEADER_IN}"
            "${VERSION_HEADER_OUT}"
            @ONLY
        )
    endif()
    
    message(STATUS "Generated version header: ${VERSION_HEADER_OUT}")
    
endfunction()
