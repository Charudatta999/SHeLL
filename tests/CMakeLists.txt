# =========================================================
# SHeLL — Tests CMakeLists.txt
# =========================================================

find_package(GTest REQUIRED)
include(GoogleTest)

# Define the unit testing executable
add_executable(shell_tests
    unit/io/PipeTest.cpp
    unit/io/FdOpsReadByteTest.cpp
    unit/signals/SigchldTest.cpp
    unit/parser/TokenTest.cpp
    unit/parser/ParserTest.cpp
    unit/parser/AstPrinterTest.cpp
    unit/shell/ShellStateTest.cpp
    unit/shell/ExpanderTest.cpp
    unit/shell/JobTableTest.cpp
    unit/shell/ShellCompleterTest.cpp
    unit/line/CompletionPagerTest.cpp
    unit/exec/CommandSpecTest.cpp
    unit/exec/ProcessExecutorTest.cpp
    unit/exec/WaitStatusTest.cpp
    unit/exec/TaskTest.cpp
    unit/exec/RedirectionTest.cpp
    unit/exec/CaptureOutputTest.cpp
    unit/exec/ExecutorTest.cpp
    unit/exec/ArithmeticCommandTest.cpp
    unit/arithmetic/ArithmeticEngineTest.cpp
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
