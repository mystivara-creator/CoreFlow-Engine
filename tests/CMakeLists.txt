################################################################################
# tests/CMakeLists.txt
#
# Test suite configuration
# Defines all unit tests with consistent settings and discovery
################################################################################

# Test definitions: map of test_name -> {sources}
# Format: "test_name" -> ["source1.cpp", "source2.cpp", ...]
set(COREFLOW_TESTS
    policy_tests
    config_tests
    baseline_tests
    mutation_tests
)

# Test source mappings
set(coreflow_policy_tests_SOURCES
    policy_tests.cpp
    ../src/policy.cpp
)

set(coreflow_config_tests_SOURCES
    config_tests.cpp
    ../src/config.cpp
)

set(coreflow_baseline_tests_SOURCES
    baseline_intelligence_tests.cpp
    ../src/baseline_intelligence.cpp
)

set(coreflow_mutation_tests_SOURCES
    mutation_tests.cpp
    ../src/mutation.cpp
    ../src/config.cpp
)

################################################################################
# Create test executables
################################################################################
foreach(TEST_NAME ${COREFLOW_TESTS})
    set(TEST_TARGET coreflow_${TEST_NAME})
    set(TEST_SOURCES ${coreflow_${TEST_NAME}_SOURCES})
    
    # Create executable
    add_executable(${TEST_TARGET} ${TEST_SOURCES})
    
    # Set target properties
    set_target_properties(${TEST_TARGET} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
    )
    
    # Include directories
    target_include_directories(${TEST_TARGET} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    # Compiler options
    target_compile_options(${TEST_TARGET} PRIVATE
        -Wall -Wextra -Wpedantic
        -Wconversion -Wsign-conversion
        -Werror
        $<$<CONFIG:Debug>:-g3 -O0>
    )
    
    # Enable testing
    add_test(
        NAME ${TEST_TARGET}
        COMMAND ${TEST_TARGET}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin/tests
    )
    
    message(STATUS "✓ Test configured: ${TEST_TARGET}")
endforeach()

################################################################################
# Test discovery and reporting
################################################################################

# Add custom target to run all tests
add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
    DEPENDS ${COREFLOW_TESTS}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all tests..."
)

# Add custom target for test coverage (requires gcov/lcov)
option(ENABLE_COVERAGE "Enable code coverage" OFF)
if(ENABLE_COVERAGE)
    add_custom_target(coverage
        COMMAND echo "Coverage report would be generated here"
        COMMENT "Generating coverage report..."
    )
    message(STATUS "Code coverage analysis available via: make coverage")
endif()

message(STATUS "
===============================================================================
  Test Suite Configuration
===============================================================================
  Tests: ${COREFLOW_TESTS}
  Output directory: ${CMAKE_BINARY_DIR}/bin/tests
  
  Run tests with:
    ctest --verbose
  or
    make check
===============================================================================
")
