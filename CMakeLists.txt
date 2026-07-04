# =========================================================
# SHeLL — Root CMakeLists.txt
# =========================================================
# A bash-like shell and GPU-accelerated terminal emulator.
# Uses SDL2 + OpenGL 3.3 + FreeType + HarfBuzz + libvterm.
# Target: 10-16 MB RAM, runs on X11 and Wayland.
#
# Build (clean first via preset):
#   cmake --preset debug          # configure
#   cmake --build --preset debug  # clean + build
#
# Legacy (no auto-clean):
#   cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build
#
# Run:
#   ./build/bin/shell
# =========================================================

cmake_minimum_required(VERSION 3.22)

project(SHeLL
    VERSION 0.1.0
    DESCRIPTION "A bash-like shell and GPU-accelerated terminal emulator"
    LANGUAGES CXX
)

# ─── CMake Modules ────────────────────────────────────────
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(StandardProjectSettings)
include(CompilerWarnings)
include(Sanitizers)

# ─── Configure Local Third-Party Paths ───────────────────
set(THIRD_PARTY_INSTALL_DIR "${CMAKE_SOURCE_DIR}/third_party")

# Add local prefix path so find_package uses local builds before system packages
list(PREPEND CMAKE_PREFIX_PATH "${THIRD_PARTY_INSTALL_DIR}")

# Prepend pkg-config path so pkg_check_modules finds our local .pc files first
set(ENV{PKG_CONFIG_PATH} "${THIRD_PARTY_INSTALL_DIR}/lib/pkgconfig:${THIRD_PARTY_INSTALL_DIR}/share/pkgconfig:$ENV{PKG_CONFIG_PATH}")

# ─── Build options ───────────────────────────────────────
# GUI deps (SDL2/OpenGL/FreeType/HarfBuzz/vterm) are vendored in third_party/
# and not available on minimal CI images. Turn OFF to build only the shell
# backend (shell_core) + tests.
option(BUILD_GUI "Build the GUI terminal (SDL2/OpenGL/FreeType/HarfBuzz/vterm)" ON)

# ─── Find Dependencies ───────────────────────────────────
if(BUILD_GUI)
    # SDL2 — windowing, input, OpenGL context (X11 + Wayland)
    find_package(SDL2 REQUIRED)

    # OpenGL — GPU rendering (Headers provided by Glad)
    # find_package(OpenGL REQUIRED)

    # FreeType — font rasterization
    find_package(Freetype REQUIRED)

    # HarfBuzz — complex text shaping
    # Use pkg-config explicitly because the generated CMake config is broken when built locally
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(HARFBUZZ REQUIRED IMPORTED_TARGET harfbuzz)

    # libvterm — terminal escape sequence state machine
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(VTERM QUIET IMPORTED_TARGET vterm)
    endif()
    # If not found via pkg-config, try manual detection
    if(NOT VTERM_FOUND)
        find_library(VTERM_LIBRARIES NAMES vterm)
        find_path(VTERM_INCLUDE_DIRS NAMES vterm.h PATH_SUFFIXES vterm)
        if(VTERM_LIBRARIES AND VTERM_INCLUDE_DIRS)
            set(VTERM_FOUND TRUE)
        else()
            message(WARNING "libvterm not found — terminal emulator will not build. Install via: vcpkg install libvterm")
        endif()
    endif()
endif()

# RapidJSON — fast JSON parsing for MCP protocol (header-only)
find_path(RAPIDJSON_INCLUDE_DIRS NAMES rapidjson/document.h
    PATH_SUFFIXES rapidjson
    HINTS ${CMAKE_SOURCE_DIR}/third_party/rapidjson/include
)
if(RAPIDJSON_INCLUDE_DIRS)
    message(STATUS "RapidJSON found: ${RAPIDJSON_INCLUDE_DIRS}")
else()
    message(WARNING "RapidJSON not found — MCP module will not build. Install via: vcpkg install rapidjson")
endif()

# ─── Include Directories ─────────────────────────────────
set(SHELL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")

# ═════════════════════════════════════════════════════════
# Shell Backend Library (shell_core)
# ═════════════════════════════════════════════════════════
# Pure shell logic — no UI dependencies.
# Syscall-first: fork, exec, waitpid, pipe, dup2, sigaction.

add_library(shell_core STATIC
    # --- Shell Core ---
    src/shell/ShellState.cpp
    src/shell/ShellException.cpp
    src/shell/ShellArithmeticVars.cpp
    src/shell/expander/Expander.cpp
    src/shell/JobTable.cpp
    src/shell/Repl.cpp
    src/shell/Prompt.cpp

    # --- Parser ---
    src/parser/Token.cpp
    src/parser/Tokenizer.cpp
    src/parser/Parser.cpp
    src/parser/ParserException.cpp

    # --- Parser AST ---
    src/parser/ast/Ast.cpp
    src/parser/ast/AstPrinter.cpp

    # --- Executor ---
    src/exec/ExecException.cpp
    src/exec/Executor.cpp
    src/exec/Redirection.cpp
    src/exec/WaitStatus.cpp
    src/exec/ProcessExecutor.cpp
    src/exec/CaptureOutput.cpp

    # --- Line Editor ---
    src/line/Terminal.cpp
    src/line/History.cpp
    src/line/LineEditor.cpp

    # --- Arithmetic ---
    src/arithmetic/ArithmeticEngine.cpp
    src/arithmetic/ArithmeticException.cpp

    # --- Builtins ---
    src/builtins/BuiltinDispatcher.cpp
    src/builtins/Cd.cpp
    src/builtins/Exit.cpp
    src/builtins/Pwd.cpp
    src/builtins/Echo.cpp
    src/builtins/Jobs.cpp
    src/builtins/Bg.cpp
    src/builtins/Fg.cpp
    src/builtins/Wait.cpp
    src/builtins/Break.cpp
    src/builtins/Continue.cpp
    src/builtins/Return.cpp

    # --- Coroutines ---
    src/coro/Task.cpp
    src/coro/TaskPromise.cpp
    src/coro/FinalAwaiter.cpp

    # --- Signals ---
    src/signals/SignalManager.cpp
    src/signals/SignalGuard.cpp
    src/signals/Sigchld.cpp

    # --- I/O ---
    src/io/FdOps.cpp
    src/io/IOException.cpp
    src/io/Pipe.cpp
    src/io/FileDescriptor.cpp

    # --- Utils ---
    src/utils/Error.cpp
    src/utils/Logger.cpp
    src/utils/Retry.cpp
    src/utils/Json.cpp

    # --- Platform ---
    src/platform/SyscallWrappers.cpp
    src/platform/ErrnoUtils.cpp
)

target_include_directories(shell_core
    PUBLIC ${SHELL_INCLUDE_DIR}
)
if(RAPIDJSON_INCLUDE_DIRS)
    target_include_directories(shell_core PRIVATE ${RAPIDJSON_INCLUDE_DIRS})
endif()
target_set_warnings(shell_core)
target_enable_sanitizers(shell_core)

# ═════════════════════════════════════════════════════════
# SheLL REPL
# ═════════════════════════════════════════════════════════

add_executable(shellrepl src/ReplMain.cpp)
target_link_libraries(shellrepl PRIVATE shell_core)
target_include_directories(shellrepl PRIVATE ${SHELL_INCLUDE_DIR})
target_set_warnings(shellrepl)
target_enable_sanitizers(shellrepl)

# ═════════════════════════════════════════════════════════
# Terminal Emulator Library (terminal_core)
# ═════════════════════════════════════════════════════════
# PTY management, libvterm wrapper, scrollback buffer.

if(VTERM_FOUND)
    add_library(terminal_core STATIC
        src/terminal/PtyMaster.cpp
        src/terminal/TerminalState.cpp
        src/terminal/Scrollback.cpp
        src/terminal/InputHandler.cpp
        src/terminal/ColorScheme.cpp
    )

    target_include_directories(terminal_core
        PUBLIC ${SHELL_INCLUDE_DIR}
    )

    target_link_libraries(terminal_core
        PRIVATE PkgConfig::VTERM
    )

    target_set_warnings(terminal_core)
    target_enable_sanitizers(terminal_core)
endif()

# ═════════════════════════════════════════════════════════
# Renderer Library (renderer_core)
# ═════════════════════════════════════════════════════════
# Two backends compiled together, selected at runtime:
#   - OpenGL 3.3 (GPU or Mesa LLVMpipe software)
#   - SDL2 software renderer (CPU-only fallback)
# Both implement the IRenderer interface.

if(BUILD_GUI)
    add_library(renderer_core STATIC
        # --- GPU backend (OpenGL 3.3) ---
        src/renderer/GlRenderer.cpp
        src/renderer/GlyphAtlas.cpp
        src/renderer/CellRenderer.cpp
        src/renderer/Shader.cpp
        src/renderer/Cursor.cpp

        # --- CPU backend (SDL2 software fallback) ---
        src/renderer/SwRenderer.cpp

        # --- OpenGL Loader (Glad) ---
        third_party/src/glad/gl.c
    )

    target_include_directories(renderer_core
        PUBLIC ${SHELL_INCLUDE_DIR}
        PRIVATE
            ${FREETYPE_INCLUDE_DIRS}
            ${CMAKE_SOURCE_DIR}/third_party/include
    )

    target_link_libraries(renderer_core
        PRIVATE
            Freetype::Freetype
            ${CMAKE_DL_LIBS}
    )

    # Link HarfBuzz (imported via pkg-config)
    target_link_libraries(renderer_core PRIVATE PkgConfig::HARFBUZZ)

    target_set_warnings(renderer_core)
    target_enable_sanitizers(renderer_core)

    # ═════════════════════════════════════════════════════════
    # Main Executable (shell)
    # ═════════════════════════════════════════════════════════
    # SDL2 window + event loop → wires everything together.

    add_executable(shell
        src/Main.cpp
        src/app/Window.cpp
        src/app/EventLoop.cpp
        src/app/Config.cpp
    )

    target_include_directories(shell PRIVATE ${SHELL_INCLUDE_DIR})

    target_link_libraries(shell
        PRIVATE
            shell_core
            renderer_core
            SDL2::SDL2
    )

    if(VTERM_FOUND AND TARGET terminal_core)
        target_link_libraries(shell PRIVATE terminal_core)
        target_compile_definitions(shell PRIVATE SHELL_HAS_TERMINAL=1)
    endif()

    target_set_warnings(shell)
    target_enable_sanitizers(shell)
endif()

# ═════════════════════════════════════════════════════════
# Parser REPL (parser_repl) — manual parser test harness
# ═════════════════════════════════════════════════════════
# Tokenize -> parse -> print AST. No UI / SDL deps.

add_executable(parser_repl
    tools/parser_repl.cpp
)

target_include_directories(parser_repl PRIVATE ${SHELL_INCLUDE_DIR})
target_link_libraries(parser_repl PRIVATE shell_core)
target_set_warnings(parser_repl)
target_enable_sanitizers(parser_repl)

# ═════════════════════════════════════════════════════════
# MCP Module (mcp_core) — opt-in
# ═════════════════════════════════════════════════════════
# Model Context Protocol server + client.
# Requires RapidJSON for JSON-RPC 2.0 message handling.

option(BUILD_MCP "Build MCP (Model Context Protocol) support" ON)

if(BUILD_MCP AND RAPIDJSON_INCLUDE_DIRS)
    add_library(mcp_core STATIC
        src/mcp/McpServer.cpp
        src/mcp/McpClient.cpp
        src/mcp/McpTransport.cpp
        src/mcp/McpTools.cpp
        src/mcp/McpResources.cpp
        src/mcp/JsonRpc.cpp
    )

    target_include_directories(mcp_core
        PUBLIC ${SHELL_INCLUDE_DIR}
        PRIVATE ${RAPIDJSON_INCLUDE_DIRS}
    )

    target_link_libraries(mcp_core PRIVATE shell_core)
    target_set_warnings(mcp_core)
    target_enable_sanitizers(mcp_core)

    # Link MCP into main executable (only when the GUI shell exe is built)
    if(TARGET shell)
        target_link_libraries(shell PRIVATE mcp_core)
        target_compile_definitions(shell PRIVATE SHELL_HAS_MCP=1)
    endif()
    message(STATUS "MCP support enabled")
elseif(BUILD_MCP)
    message(WARNING "MCP support requested but RapidJSON not found — skipping")
endif()

# ═════════════════════════════════════════════════════════
# Copy Shaders to Build Directory
# ═════════════════════════════════════════════════════════

file(GLOB SHADER_FILES "${CMAKE_SOURCE_DIR}/shaders/*")
if(SHADER_FILES AND TARGET shell)
    add_custom_command(TARGET shell POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/shaders"
            "$<TARGET_FILE_DIR:shell>/shaders"
        COMMENT "Copying shaders to build directory"
    )
endif()

# ═════════════════════════════════════════════════════════
# Tests (opt-in)
# ═════════════════════════════════════════════════════════

option(BUILD_TESTING "Build unit and integration tests" OFF)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

# ═════════════════════════════════════════════════════════
# Benchmarks (opt-in)
# ═════════════════════════════════════════════════════════

option(BUILD_BENCHMARKS "Build performance benchmarks" OFF)

if(BUILD_BENCHMARKS)
    # add_subdirectory(benchmarks)
endif()

# ─── Build Summary ────────────────────────────────────────
message(STATUS "")
message(STATUS "══════════════════════════════════════")
message(STATUS "  SHeLL v${PROJECT_VERSION} Build Summary")
message(STATUS "══════════════════════════════════════")
message(STATUS "  Build Type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler:     ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Sanitizers:   ${ENABLE_SANITIZERS}")
message(STATUS "  SDL2:         found")
message(STATUS "  OpenGL:       found")
message(STATUS "  FreeType:     found")
message(STATUS "  HarfBuzz:     ${harfbuzz_FOUND}")
message(STATUS "  libvterm:     ${VTERM_FOUND}")
message(STATUS "  RapidJSON:    ${RAPIDJSON_INCLUDE_DIRS}")
message(STATUS "  MCP:          ${BUILD_MCP}")
message(STATUS "  Tests:        ${BUILD_TESTING}")
message(STATUS "  Benchmarks:   ${BUILD_BENCHMARKS}")
message(STATUS "══════════════════════════════════════")
message(STATUS "")
