# CoreFlow dynamic source packaging.
#
# No source file is listed by name anywhere in the build. Adding a file under
# src/ or a test under tests/ requires no CMake edit: the next configure picks it
# up (CONFIGURE_DEPENDS re-runs the glob on build).
#
# Platform rule, evaluated per file:
#   1. An explicit tag in the first 40 lines wins:
#        // @coreflow-platform: any      (host and device)
#        // @coreflow-platform: android  (device daemon only)
#   2. Otherwise a file that includes an Android platform header
#      (<android/...> or <sys/system_properties.h>) is device-only.
#   3. Otherwise it is platform-neutral and belongs to coreflow_core.
# A file that includes a platform header but is meant for host too must carry an
# explicit `any` tag, and then it must not include that header unconditionally.

function(coreflow_platform_of source out_var)
    file(STRINGS "${source}" _head LIMIT_COUNT 40)
    set(_platform "")
    foreach(_line IN LISTS _head)
        if(_line MATCHES "@coreflow-platform:[ ]*([a-z]+)")
            set(_platform "${CMAKE_MATCH_1}")
            break()
        endif()
    endforeach()

    if(_platform STREQUAL "")
        file(STRINGS "${source}" _platform_includes
             REGEX "^[ ]*#[ ]*include[ ]*[<\"](android/|sys/system_properties\\.h)")
        if(_platform_includes)
            set(_platform "android")
        else()
            set(_platform "any")
        endif()
    endif()

    if(NOT _platform MATCHES "^(any|android)$")
        message(FATAL_ERROR "Unknown @coreflow-platform tag '${_platform}' in ${source}")
    endif()
    set(${out_var} "${_platform}" PARENT_SCOPE)
endfunction()

function(coreflow_collect_sources)
    file(GLOB_RECURSE _candidates CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.cpp")
    set(_core "")
    set(_android "")
    foreach(_src IN LISTS _candidates)
        coreflow_platform_of("${_src}" _platform)
        if(_platform STREQUAL "android")
            list(APPEND _android "${_src}")
        else()
            list(APPEND _core "${_src}")
        endif()
    endforeach()
    set(COREFLOW_CORE_SOURCES "${_core}" PARENT_SCOPE)
    set(COREFLOW_ANDROID_SOURCES "${_android}" PARENT_SCOPE)
endfunction()

coreflow_collect_sources()
