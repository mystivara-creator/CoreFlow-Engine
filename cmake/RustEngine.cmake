find_program(COREFLOW_CARGO NAMES cargo cargo-1.85 REQUIRED)
set(COREFLOW_RUST_TARGET "" CACHE STRING "Rust target triple (empty for native)")
set(_cargo_args)
set(_cargo_subdir "")
if(ANDROID)
    if(NOT ANDROID_ABI STREQUAL "arm64-v8a")
        message(FATAL_ERROR "Rust bridge currently supports Android arm64-v8a only")
    endif()
    set(COREFLOW_RUST_TARGET "aarch64-linux-android")
endif()
if(COREFLOW_RUST_TARGET)
    list(APPEND _cargo_args --target "${COREFLOW_RUST_TARGET}")
    set(_cargo_subdir "${COREFLOW_RUST_TARGET}/")
endif()
set(_cargo_dir "${CMAKE_BINARY_DIR}/cargo")
set(_rust_lib "${_cargo_dir}/${_cargo_subdir}release/libcoreflow_engine.a")
set(_rust_env "CARGO_TARGET_DIR=${_cargo_dir}" "CXX=${CMAKE_CXX_COMPILER}")
if(ANDROID)
    # The cxx build script compiles the bridge using the same NDK ABI/API as CMake.
    list(APPEND _rust_env "CXXFLAGS=--target=${CMAKE_CXX_COMPILER_TARGET} --sysroot=${CMAKE_SYSROOT} -stdlib=libc++")
    list(APPEND _rust_env "AR=${CMAKE_AR}")
    list(APPEND _rust_env "CARGO_TARGET_AARCH64_LINUX_ANDROID_LINKER=${CMAKE_CXX_COMPILER}")
endif()
add_custom_command(OUTPUT "${_rust_lib}"
    COMMAND ${CMAKE_COMMAND} -E env ${_rust_env}
        "${COREFLOW_CARGO}" build --manifest-path "${CMAKE_SOURCE_DIR}/rust/Cargo.toml"
        --release ${_cargo_args}
    DEPENDS rust/Cargo.toml rust/build.rs rust/src/lib.rs rust/src/bridge.rs rust/src/controller.rs
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    VERBATIM)
add_custom_target(coreflow_rust_build DEPENDS "${_rust_lib}")
add_library(coreflow_rust STATIC IMPORTED GLOBAL)
set_target_properties(coreflow_rust PROPERTIES IMPORTED_LOCATION "${_rust_lib}")
file(MAKE_DIRECTORY "${_cargo_dir}/cxxbridge")
target_include_directories(coreflow_rust INTERFACE "${_cargo_dir}/cxxbridge")
add_dependencies(coreflow_rust coreflow_rust_build)
