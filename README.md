<div align="center">

# SHeLL

**A lightweight, GPU-accelerated terminal emulator and bash-like shell**

Built from scratch in C++ using raw POSIX syscalls, OpenGL rendering, and the Model Context Protocol.

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-brightgreen.svg)](https://en.cppreference.com/w/cpp/20)
[![Build System](https://img.shields.io/badge/Build-CMake-red.svg)](CMakeLists.txt)

</div>

---

## Features

### Terminal Emulator
- **GPU-accelerated rendering** via OpenGL 3.3 — entire screen in one draw call
- **Software fallback** — works on machines without a GPU (Mesa LLVMpipe or SDL2 CPU renderer)
- **X11 + Wayland** — auto-detected at runtime via SDL2
- **FreeType + HarfBuzz** — font rendering with ligature support
- **Truecolor** — full 24-bit color, 256-color, and ANSI 16
- **10-16 MB RAM** — lightweight enough for resource-scarce machines

### Shell
- **Syscall-first** — directly models `fork`, `execve`, `waitpid`, `pipe`, `dup2`, `sigaction`
- **RAII file descriptors** — no fd leaks, no double-close bugs
- **Pipelines** — `cmd1 | cmd2 | cmd3`
- **I/O redirection** — `>`, `<`, `>>`
- **Builtins** — `cd`, `exit`, `pwd`, `echo`
- **Signal handling** — SIGINT, SIGCHLD, EINTR retry loops
- **Observability** — designed for verification with `strace`, `valgrind`, ASAN, UBSAN

### MCP (Model Context Protocol)
- **MCP Server** — expose shell capabilities to AI assistants (Claude, Cursor, VS Code)
- **MCP Client** — connect to external MCP servers from the shell
- **AI Assist** — `?? natural language` → suggested shell commands
- **JSON-RPC 2.0** over stdio — zero network dependencies

---

## Architecture

```
┌──────────────────────────────────────────────┐
│  SDL2 Window  (X11 / Wayland)                │
│  ┌────────────────────────────────────────┐  │
│  │  Renderer  (OpenGL GPU / SDL2 CPU)     │  │
│  │  ┌──────────────────────────────────┐  │  │
│  │  │  Glyph Atlas (FreeType+HarfBuzz) │  │  │
│  │  │  Cell Grid  (instanced drawing)  │  │  │
│  │  └──────────────────────────────────┘  │  │
│  └────────────────────────────────────────┘  │
│                  ↕ VT escape sequences        │
│  ┌────────────────────────────────────────┐  │
│  │  libvterm  (terminal state machine)    │  │
│  └────────────────────────────────────────┘  │
│                  ↕ PTY read/write             │
│  ┌────────────────────────────────────────┐  │
│  │  Shell Backend                         │  │
│  │  REPL → Parser → Executor → fork/exec │  │
│  └────────────────────────────────────────┘  │
│                  ↕ JSON-RPC (stdio)           │
│  ┌────────────────────────────────────────┐  │
│  │  MCP  (server + client)                │  │
│  └────────────────────────────────────────┘  │
└──────────────────────────────────────────────┘
```

---

## Build

### Prerequisites

```bash
# Arch Linux
sudo pacman -S sdl2 mesa freetype2 harfbuzz libvterm rapidjson

# Ubuntu / Debian
sudo apt install libsdl2-dev libgl-dev libfreetype-dev libharfbuzz-dev libvterm-dev rapidjson-dev

# Fedora
sudo dnf install SDL2-devel mesa-libGL-devel freetype-devel harfbuzz-devel libvterm-devel rapidjson-devel
```

### Compile

```bash
# Debug (with sanitizers)
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)

# Release (optimized)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Run

```bash
# Terminal emulator mode (default)
./build/bin/shell

# MCP server mode (for AI assistants)
./build/bin/shell --mcp-server
```

### CMake Options

| Flag | Default | Description |
|------|---------|-------------|
| `CMAKE_BUILD_TYPE` | `Debug` | `Debug`, `Release`, `RelWithDebInfo`, `MinSizeRel` |
| `ENABLE_SANITIZERS` | `ON` | AddressSanitizer + UBSan in Debug builds |
| `BUILD_MCP` | `ON` | MCP (Model Context Protocol) support |
| `BUILD_TESTING` | `OFF` | Unit and integration tests |
| `BUILD_BENCHMARKS` | `OFF` | Performance benchmarks |

---

## Project Structure

```
SHeLL/
├── include/
│   ├── shell/        Shell state, REPL, prompt
│   ├── parser/       Tokenizer, parser, command model
│   ├── exec/         Executor, process, pipeline, redirection
│   ├── builtins/     cd, exit, echo, pwd, dispatcher
│   ├── signals/      Signal manager, guard, SIGCHLD
│   ├── io/           FD wrapper, pipe, cloexec
│   ├── utils/        Error, logger, retry, Json wrapper
│   ├── platform/     Syscall wrappers, errno utils
│   ├── terminal/     PTY master, terminal state, scrollback
│   ├── renderer/     OpenGL/software renderer, glyph atlas
│   ├── app/          SDL2 window, event loop, config
│   └── mcp/          MCP server, client, transport, tools
│
├── src/              Source files (mirrors include/ layout)
├── shaders/          GLSL vertex/fragment shaders
├── cmake/            CMake modules (warnings, sanitizers)
├── docs/             Architecture docs, dependency guide
├── tests/            Unit, integration, fuzz tests
├── benchmarks/       Startup, parsing, execution benchmarks
├── scripts/          Dev scripts (format, lint, strace, valgrind)
└── examples/         Example shell scripts
```

---

## Libraries

| Library | Purpose | Required |
|---------|---------|----------|
| **SDL2** | Windowing, input, OpenGL context (X11 + Wayland) | Yes |
| **OpenGL 3.3** | GPU-accelerated terminal rendering | Yes (Mesa fallback on CPU) |
| **FreeType** | Font loading and glyph rasterization | Yes |
| **HarfBuzz** | Text shaping and ligatures | Recommended |
| **libvterm** | VT100/xterm escape sequence parsing | Yes |
| **RapidJSON** | JSON parsing for MCP protocol | Yes (if MCP enabled) |

> Full dependency guide with build flags, install commands, and documentation links:
> [docs/DEPENDENCIES.md](docs/DEPENDENCIES.md)

---

## Rendering

SHeLL runs on machines **with or without a GPU**:

| Backend | When Used | Performance |
|---------|-----------|-------------|
| **OpenGL 3.3 (GPU)** | Discrete/integrated GPU | ★★★★★ 60 FPS |
| **OpenGL 3.3 (LLVMpipe)** | No GPU, Mesa installed | ★★★☆☆ 30-60 FPS |
| **SDL2 Software** | No GPU, no Mesa | ★★☆☆☆ 15-30 FPS |

The backend is auto-detected at startup. Same binary, any machine.

---

## MCP Integration

### As MCP Server

AI assistants can control SHeLL via the Model Context Protocol:

```json
{
  "mcpServers": {
    "shell": {
      "command": "/usr/local/bin/shell",
      "args": ["--mcp-server"]
    }
  }
}
```

**Exposed tools:** `execute_command`, `read_file`, `write_file`, `list_directory`, `get_environment`

### As MCP Client

Connect to external MCP servers from within the shell:

```bash
mysh [0]> mcp connect sqlite
mysh [0]> mcp call sqlite query "SELECT * FROM users"
```

### AI Assist

Natural language to shell commands:

```bash
mysh [0]> ?? find all log files larger than 100MB
# AI suggests: find /var/log -name "*.log" -size +100M
# [Enter] Run  [e] Edit  [q] Cancel
```

---

## Design Principles

| Principle | Description |
|-----------|-------------|
| **Syscall First** | Understand and directly model every POSIX syscall |
| **Explicit Ownership** | Every FD, process, and pipe has a clear owner |
| **Observability First** | All behavior verifiable with strace, valgrind, ASAN |
| **Lightweight** | 10-16 MB RAM target, runs on resource-scarce machines |
| **GPU Optional** | Works identically with or without GPU hardware |

---

## Documentation

- [Dependency Guide](docs/DEPENDENCIES.md) — libraries, build flags, install commands
- [Roadmap](roadmap.md) — vision and engineering principles
- [Architecture](docs/architecture/) — shell runtime, process model, signal handling, parser design

---

## License

[MIT](LICENSE) — Copyright (c) 2026 Charudatta Jadhav