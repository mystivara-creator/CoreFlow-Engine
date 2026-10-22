cmake_minimum_required(VERSION 3.22)
project(CoreFlowEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE CORE_FLOW_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
add_executable(coreflow_daemon ${CORE_FLOW_SOURCES})
target_include_directories(coreflow_daemon PRIVATE include)
target_compile_options(coreflow_daemon PRIVATE -Wall -Wextra -Wpedantic)

if(ANDROID)
    target_link_libraries(coreflow_daemon log)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(strategy_catalog_tests
        tests/strategy_catalog_tests.cpp
        src/discovery/strategy_catalog.cpp)
    target_include_directories(strategy_catalog_tests PRIVATE include)
    target_compile_options(strategy_catalog_tests PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME strategy_catalog_tests COMMAND strategy_catalog_tests)

    add_executable(architecture_contract_tests tests/architecture_contract_tests.cpp)
    target_include_directories(architecture_contract_tests PRIVATE include)
    add_test(NAME architecture_contract_tests COMMAND architecture_contract_tests)
    add_executable(optimization_feedback_tests
        tests/optimization_feedback_tests.cpp
        src/optimization/optimization.cpp)
    target_include_directories(optimization_feedback_tests PRIVATE include)
    target_compile_options(optimization_feedback_tests PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME optimization_feedback_tests COMMAND optimization_feedback_tests)
    add_executable(mutation_safety_tests
        tests/mutation_safety_tests.cpp
        src/mutation/mutation.cpp
        src/validation/validation.cpp)
    target_include_directories(mutation_safety_tests PRIVATE include)
    target_compile_options(mutation_safety_tests PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME mutation_safety_tests COMMAND mutation_safety_tests)

    add_executable(device_profile_tests
        tests/device_profile_tests.cpp
        src/profile/device_profile.cpp
    )
    target_include_directories(device_profile_tests PRIVATE include)
    add_test(NAME device_profile_tests COMMAND device_profile_tests)

    add_executable(outcome_feedback_tests
        tests/outcome_feedback_tests.cpp
        src/outcome/outcome.cpp)
    target_include_directories(outcome_feedback_tests PRIVATE include)
    target_compile_options(outcome_feedback_tests PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME outcome_feedback_tests COMMAND outcome_feedback_tests)

endif()
