# =========================================================
# SHeLL — Tests CMakeLists.txt
# =========================================================

find_package(GTest REQUIRED)
include(GoogleTest)

# Define the unit testing executable
add_executable(shell_tests
    unit/io/PipeTest.cpp
    # Add other test files here in the future
)

# Include the main project headers
target_include_directories(shell_tests
    PRIVATE ${SHELL_INCLUDE_DIR}
)

# Link against the shell_core library and GTest
target_link_libraries(shell_tests
    PRIVATE
        shell_core
        GTest::gtest_main
)

# Register tests with CTest
gtest_discover_tests(shell_tests)

target_set_warnings(shell_tests)
target_enable_sanitizers(shell_tests)
