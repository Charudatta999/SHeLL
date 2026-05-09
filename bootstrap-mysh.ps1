# =========================================================
# MYSH Project Structure Bootstrap Script
# =========================================================

$Root = "mysh"

$Directories = @(
    ".github/workflows",

    "cmake",

    "docs/architecture",
    "docs/sprint",
    "docs/debugging",
    "docs/notes",

    "include/mysh/shell",
    "include/mysh/parser",
    "include/mysh/exec",
    "include/mysh/builtins",
    "include/mysh/signals",
    "include/mysh/io",
    "include/mysh/utils",
    "include/mysh/platform",

    "src/shell",
    "src/parser",
    "src/exec",
    "src/builtins",
    "src/signals",
    "src/io",
    "src/utils",
    "src/platform",

    "tests/unit/parser",
    "tests/unit/exec",
    "tests/unit/builtins",
    "tests/unit/utils",

    "tests/integration/repl",
    "tests/integration/process",
    "tests/integration/signals",
    "tests/integration/pipelines",

    "tests/fixtures/scripts",
    "tests/fixtures/inputs",

    "tests/fuzz",

    "scripts",

    "benchmarks/startup",
    "benchmarks/parsing",
    "benchmarks/execution",

    "third_party",

    "assets/diagrams",
    "assets/screenshots",

    "examples",

    "build"
)

$Files = @(
    ".github/workflows/ci.yml",
    ".github/workflows/sanitizers.yml",
    ".github/workflows/clang-tidy.yml",

    "cmake/CompilerWarnings.cmake",
    "cmake/Sanitizers.cmake",
    "cmake/StandardProjectSettings.cmake",

    "docs/architecture/shell-runtime.md",
    "docs/architecture/process-model.md",
    "docs/architecture/signal-handling.md",
    "docs/architecture/parser-design.md",
    "docs/architecture/fd-ownership.md",

    "docs/sprint/sprint-1.md",
    "docs/sprint/sprint-2.md",
    "docs/sprint/roadmap.md",

    "docs/debugging/strace-guide.md",
    "docs/debugging/gdb-guide.md",
    "docs/debugging/valgrind.md",
    "docs/debugging/sanitizers.md",

    "docs/notes/unix-semantics.md",
    "docs/notes/linux-syscalls.md",
    "docs/notes/shell-behavior.md",

    "include/mysh/shell/shell_state.hpp",
    "include/mysh/shell/repl.hpp",
    "include/mysh/shell/prompt.hpp",

    "include/mysh/parser/tokenizer.hpp",
    "include/mysh/parser/parser.hpp",
    "include/mysh/parser/token.hpp",
    "include/mysh/parser/command.hpp",

    "include/mysh/exec/executor.hpp",
    "include/mysh/exec/process.hpp",
    "include/mysh/exec/pipeline.hpp",
    "include/mysh/exec/redirection.hpp",
    "include/mysh/exec/wait_status.hpp",

    "include/mysh/builtins/builtin_dispatcher.hpp",
    "include/mysh/builtins/cd.hpp",
    "include/mysh/builtins/exit.hpp",
    "include/mysh/builtins/pwd.hpp",
    "include/mysh/builtins/echo.hpp",

    "include/mysh/signals/signal_manager.hpp",
    "include/mysh/signals/signal_guard.hpp",
    "include/mysh/signals/sigchld.hpp",

    "include/mysh/io/fd.hpp",
    "include/mysh/io/pipe.hpp",
    "include/mysh/io/file_descriptor.hpp",
    "include/mysh/io/cloexec.hpp",

    "include/mysh/utils/expected.hpp",
    "include/mysh/utils/string_utils.hpp",
    "include/mysh/utils/error.hpp",
    "include/mysh/utils/logger.hpp",
    "include/mysh/utils/retry.hpp",

    "include/mysh/platform/syscall_wrappers.hpp",
    "include/mysh/platform/errno_utils.hpp",

    "src/main.cpp",

    "src/shell/repl.cpp",
    "src/shell/prompt.cpp",
    "src/shell/shell_state.cpp",

    "src/parser/tokenizer.cpp",
    "src/parser/parser.cpp",
    "src/parser/command.cpp",

    "src/exec/executor.cpp",
    "src/exec/process.cpp",
    "src/exec/pipeline.cpp",
    "src/exec/redirection.cpp",
    "src/exec/wait_status.cpp",

    "src/builtins/builtin_dispatcher.cpp",
    "src/builtins/cd.cpp",
    "src/builtins/exit.cpp",
    "src/builtins/pwd.cpp",
    "src/builtins/echo.cpp",

    "src/signals/signal_manager.cpp",
    "src/signals/signal_guard.cpp",
    "src/signals/sigchld.cpp",

    "src/io/fd.cpp",
    "src/io/pipe.cpp",
    "src/io/file_descriptor.cpp",
    "src/io/cloexec.cpp",

    "src/utils/logger.cpp",
    "src/utils/retry.cpp",
    "src/utils/error.cpp",

    "src/platform/syscall_wrappers.cpp",
    "src/platform/errno_utils.cpp",

    "tests/fuzz/parser_fuzz.cpp",

    "scripts/run.sh",
    "scripts/format.sh",
    "scripts/lint.sh",
    "scripts/strace.sh",
    "scripts/valgrind.sh",
    "scripts/coverage.sh",

    "examples/simple.sh",
    "examples/pipes.sh",
    "examples/signals.sh",

    ".clang-format",
    ".clang-tidy",
    ".gitignore",
    "CMakeLists.txt",
    "README.md",
    "LICENSE",
    "roadmap.md"
)

Write-Host ""
Write-Host "======================================="
Write-Host " Creating MYSH Project Structure"
Write-Host "======================================="
Write-Host ""

# Create root
New-Item -ItemType Directory -Path $Root -Force | Out-Null

# Create directories
foreach ($Dir in $Directories)
{
    $FullPath = Join-Path $Root $Dir

    if (-not (Test-Path $FullPath))
    {
        New-Item -ItemType Directory -Path $FullPath -Force | Out-Null
        Write-Host "[DIR ] $FullPath"
    }
}

# Create files
foreach ($File in $Files)
{
    $FullPath = Join-Path $Root $File

    if (-not (Test-Path $FullPath))
    {
        New-Item -ItemType File -Path $FullPath -Force | Out-Null
        Write-Host "[FILE] $FullPath"
    }
}

Write-Host ""
Write-Host "======================================="
Write-Host " MYSH bootstrap complete"
Write-Host "======================================="
Write-Host ""
