################################################################################
# cmake/compiler_flags.cmake
# 
# Centralized compiler & linker flag configuration
# Provides proper warnings, optimizations, and hardening
################################################################################

function(coreflow_set_compiler_flags)
    # Common C++ flags for all targets
    set(COREFLOW_COMMON_FLAGS
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wcast-align
        -Wdouble-promotion
        -Wunused
        -Wnull-dereference
        -Wformat=2
        -Wundef
        -Werror
    )
    
    # Debug flags
    set(COREFLOW_DEBUG_FLAGS
        -g3
        -O0
        -D_DEBUG
        -DCOREFLOW_DEBUG=1
    )
    
    # Release/Optimized flags
    set(COREFLOW_RELEASE_FLAGS
        -O3
        -DNDEBUG
        -ffast-math
        -march=native
    )
    
    # Security hardening flags (all builds)
    set(COREFLOW_HARDENING_FLAGS
        -D_FORTIFY_SOURCE=2
        -fstack-protector-strong
        -fstack-clash-protection
        -fno-strict-aliasing
        -fno-common
    )
    
    # Platform-specific Android hardening
    if(ANDROID)
        list(APPEND COREFLOW_HARDENING_FLAGS
            -fPIE
            -ffunction-sections
            -fdata-sections
            -Wl,--gc-sections
        )
        set(COREFLOW_ANDROID_LINK_FLAGS
            -pie
            -Wl,-z,relro
            -Wl,-z,now
            -Wl,-z,text
            -Wl,-z,noexecstack
            -Wl,-z,separate-code
        )
        set(COREFLOW_ANDROID_LINK_FLAGS ${COREFLOW_ANDROID_LINK_FLAGS} PARENT_SCOPE)
    endif()
    
    # Apply to CMAKE variables (affects all targets)
    add_compile_options(${COREFLOW_COMMON_FLAGS})
    add_compile_options($<$<CONFIG:Debug>:${COREFLOW_DEBUG_FLAGS}>)
    add_compile_options($<$<CONFIG:Release>:${COREFLOW_RELEASE_FLAGS}>)
    add_compile_options(${COREFLOW_HARDENING_FLAGS})
    
    # Link flags for hardening
    add_link_options(${COREFLOW_HARDENING_FLAGS})
    if(ANDROID)
        add_link_options(${COREFLOW_ANDROID_LINK_FLAGS})
    endif()
    
    message(STATUS "✓ Compiler flags configured")
    message(STATUS "  Common warnings: ${COREFLOW_COMMON_FLAGS}")
    message(STATUS "  Hardening: ${COREFLOW_HARDENING_FLAGS}")
    
endfunction()

################################################################################
# Apply target-specific compilation options
################################################################################
function(coreflow_target_set_flags TARGET_NAME)
    target_compile_options(${TARGET_NAME} PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:
            -Wctor-dtor-privacy
            -Wnon-virtual-dtor
            -Wold-style-cast
            -Woverloaded-virtual
            -Wsuggest-override
    )
endfunction()
