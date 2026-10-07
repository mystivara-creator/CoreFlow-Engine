################################################################################
# cmake/android_hardening.cmake
#
# Android-specific security hardening configuration
# Applies PIE, RELRO, and other Android security best practices
################################################################################

function(coreflow_apply_android_hardening TARGET_NAME)
    if(NOT ANDROID)
        message(WARNING "Android hardening requested on non-Android platform")
        return()
    endif()
    
    message(STATUS "Applying Android security hardening to ${TARGET_NAME}")
    
    # PIE (Position Independent Executable) - required on modern Android
    set_target_properties(${TARGET_NAME} PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
    
    # Compile options for hardening
    target_compile_options(${TARGET_NAME} PRIVATE
        -fstack-protector-strong          # Stack smashing protection
        -fPIE                              # Position Independent Executable
        -ffunction-sections                # Function sections for linker GC
        -fdata-sections                    # Data sections for linker GC
        -ftrivial-auto-var-init=zero       # Initialize local variables to zero (C++20+)
        -fno-strict-overflow               # Disable unsafe overflow optimizations
    )
    
    # Link options for hardening
    target_link_options(${TARGET_NAME} PRIVATE
        -pie                               # Enable PIE linking
        -Wl,-z,relro                       # Read-only relocations
        -Wl,-z,now                         # Disable lazy binding
        -Wl,-z,noexecstack                 # Disable executable stack
        -Wl,-z,text                        # Force no relocations in read-only segments
        -Wl,-z,separate-code               # Separate code and data
        -Wl,--gc-sections                  # Garbage collect unused sections
        -Wl,--as-needed                    # Only link needed libraries
        -Wl,--no-undefined                 # Catch undefined symbols at link time
    )
    
    # Create version script to control symbol visibility (optional but recommended)
    get_target_property(TARGET_TYPE ${TARGET_NAME} TYPE)
    if(TARGET_TYPE STREQUAL "EXECUTABLE")
        # For executables, we might want to explicitly hide symbols
        target_compile_options(${TARGET_NAME} PRIVATE -fvisibility=hidden)
        target_compile_options(${TARGET_NAME} PRIVATE -fvisibility-inlines-hidden)
    endif()
    
    message(STATUS "  ✓ Stack protector: strong")
    message(STATUS "  ✓ PIE: enabled")
    message(STATUS "  ✓ RELRO/BIND_NOW: enabled")
    message(STATUS "  ✓ Executable stack: disabled")
    message(STATUS "  ✓ Code/data separation: enabled")
    
endfunction()

################################################################################
# Verify Android configuration
################################################################################
function(coreflow_verify_android_configuration)
    if(NOT ANDROID)
        return()
    endif()
    
    message(STATUS "Android Configuration:")
    message(STATUS "  Platform API Level: ${ANDROID_PLATFORM}")
    message(STATUS "  Native API Level: ${ANDROID_NATIVE_API_LEVEL}")
    message(STATUS "  ABI: ${ANDROID_ABI}")
    message(STATUS "  Toolchain: ${ANDROID_TOOLCHAIN}")
    message(STATUS "  STL: ${ANDROID_STL}")
    
    # Validate minimum API level
    if(ANDROID_NATIVE_API_LEVEL LESS 21)
        message(WARNING "Android API level ${ANDROID_NATIVE_API_LEVEL} is below recommended (21)")
    endif()
    
endfunction()
