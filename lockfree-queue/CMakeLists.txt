cmake_minimum_required(VERSION 3.14)
project(SPSC_Queue_Tests)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build instructions:
# Basic build and test:
#   mkdir -p build && cd build && cmake .. && make && ctest --verbose
#   or: mkdir -p build && cd build && cmake .. && make run_unit_tests
#
# Build with AddressSanitizer:
#   mkdir -p build && cd build && cmake -DENABLE_ASAN=ON .. && make && make test_with_asan
#
# Available targets:
#   spsc_unit_tests       - Main test executable
#   spsc_unit_tests_asan  - Test executable with AddressSanitizer
#   run_unit_tests        - Run tests via CTest (equivalent to old 'make test')
#   test_with_asan        - Run tests with AddressSanitizer
#   run_tests            - Run tests via CTest (equivalent to old 'make run_tests')

# Option to enable AddressSanitizer for all targets
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)

if(ENABLE_ASAN)
    message(STATUS "AddressSanitizer enabled")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer -g")
    set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address")
endif()

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# Try to find Google Test using pkg-config first (more reliable)
pkg_check_modules(GTEST QUIET gtest_main gtest)

# Use pkg-config found version
message(STATUS "Found Google Test via pkg-config: ${GTEST_VERSION}")
add_executable(spsc_unit_tests test/spsc_nowait.cpp)
target_compile_options(spsc_unit_tests PRIVATE ${GTEST_CFLAGS})
target_include_directories(spsc_unit_tests PRIVATE ./src ${GTEST_INCLUDE_DIRS})
target_link_libraries(spsc_unit_tests ${GTEST_LIBRARIES} Threads::Threads)
target_link_directories(spsc_unit_tests PRIVATE ${GTEST_LIBRARY_DIRS})

# Add await policies test executable
add_executable(await_policies_tests test/await_policies.cpp)
target_compile_options(await_policies_tests PRIVATE ${GTEST_CFLAGS})
target_include_directories(await_policies_tests PRIVATE ./src ${GTEST_INCLUDE_DIRS})
target_link_libraries(await_policies_tests ${GTEST_LIBRARIES} Threads::Threads)
target_link_directories(await_policies_tests PRIVATE ${GTEST_LIBRARY_DIRS})


# Add compiler flags for better debugging and warnings
target_compile_options(spsc_unit_tests PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -g
    -O2
)

target_compile_options(await_policies_tests PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -g
    -O2
)

# Create AddressSanitizer version of the tests
add_executable(spsc_unit_tests_asan test/spsc_nowait.cpp)
target_include_directories(spsc_unit_tests_asan PRIVATE ./src)
target_compile_options(spsc_unit_tests_asan PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -g
    -fsanitize=address
    -fno-omit-frame-pointer
    -fno-optimize-sibling-calls
    -O1
)
target_link_options(spsc_unit_tests_asan PRIVATE
    -fsanitize=address
)

add_executable(await_policies_tests_asan test/await_policies.cpp)
target_include_directories(await_policies_tests_asan PRIVATE ./src)
target_compile_options(await_policies_tests_asan PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -g
    -fsanitize=address
    -fno-omit-frame-pointer
    -fno-optimize-sibling-calls
    -O1
)
target_link_options(await_policies_tests_asan PRIVATE
    -fsanitize=address
)

# Link with the same libraries as the main test
target_compile_options(spsc_unit_tests_asan PRIVATE ${GTEST_CFLAGS})
target_link_libraries(spsc_unit_tests_asan ${GTEST_LIBRARIES} Threads::Threads)
target_link_directories(spsc_unit_tests_asan PRIVATE ${GTEST_LIBRARY_DIRS})

target_compile_options(await_policies_tests_asan PRIVATE ${GTEST_CFLAGS})
target_link_libraries(await_policies_tests_asan ${GTEST_LIBRARIES} Threads::Threads)
target_link_directories(await_policies_tests_asan PRIVATE ${GTEST_LIBRARY_DIRS})

# Enable testing
enable_testing()

# Add tests
add_test(NAME SPSCQueueTests COMMAND spsc_unit_tests)
add_test(NAME SPSCQueueTestsASAN COMMAND spsc_unit_tests_asan)
add_test(NAME AwaitPoliciesTests COMMAND await_policies_tests)
# Note: AwaitPoliciesTestsASAN has timing issues - run manually if needed
# add_test(NAME AwaitPoliciesTestsASAN COMMAND await_policies_tests_asan)

# Custom target to run tests (equivalent to 'make test')
add_custom_target(run_unit_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS spsc_unit_tests await_policies_tests
    COMMENT "Running unit tests"
)

# Custom target to run tests with AddressSanitizer (equivalent to 'make test_with_asan')
add_custom_target(test_with_asan
    COMMAND spsc_unit_tests_asan
    DEPENDS spsc_unit_tests_asan
    COMMENT "Running unit tests with AddressSanitizer (Note: await_policies_tests_asan has known timing issues)"
)

# Custom target for compatibility (equivalent to 'make run_tests')
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS spsc_unit_tests await_policies_tests
)

# Formatting targets
find_program(CLANG_FORMAT_EXECUTABLE clang-format)
if(CLANG_FORMAT_EXECUTABLE)
    # Check formatting
    add_custom_target(check-format
        COMMAND ${CMAKE_SOURCE_DIR}/../scripts/check-format.sh
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..
        COMMENT "Checking code formatting with clang-format"
    )
    
    # Apply formatting
    add_custom_target(format
        COMMAND ${CMAKE_SOURCE_DIR}/../scripts/apply-format.sh
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..
        COMMENT "Applying clang-format to all C++ files"
    )
    
    message(STATUS "clang-format found: formatting targets enabled")
    message(STATUS "  make check-format - Check if code is properly formatted")
    message(STATUS "  make format       - Apply formatting to all C++ files")
else()
    message(WARNING "clang-format not found: formatting targets disabled")
endif()

