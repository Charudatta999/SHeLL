# SHeLL — Third-Party Dependencies

> Terminal emulator + bash-like shell with GPU acceleration (optional).
> Target: 10-16 MB RAM | X11 + Wayland via SDL2 | Works with or without a GPU.

---

## Table of Contents

- [Feature Matrix](#feature-matrix)
- [Required Libraries](#required-libraries-must-have)
- [Recommended Libraries](#recommended-libraries-good-to-have)
- [Optional Libraries](#optional-libraries-nice-to-have)
- [Build Flags & CMake Options](#build-flags--cmake-options)
- [Compiler Flags](#compiler-flags)
- [Quick Install](#quick-install-all-required)
- [Build Instructions](#build-instructions)
- [Dependency Graph](#dependency-graph)

---

## Feature Matrix

### 🔴 Must-Have (Core — Without these, nothing works)

| Feature | Library | Why It's Required |
|---------|---------|-------------------|
| Window creation (X11 + Wayland) | **SDL2** | No window = no terminal UI |
| Keyboard & mouse input | **SDL2** | Cannot type commands without input handling |
| OpenGL context | **SDL2** | Required to initialize GPU rendering |
| GPU text rendering | **OpenGL 3.3** | Renders the terminal cell grid at 60 FPS |
| Font loading & glyph rasterization | **FreeType** | Cannot display text without glyph bitmaps |
| VT100/xterm escape sequence parsing | **libvterm** | Terminal is useless without escape code support |
| Process execution | POSIX (`fork`, `exec`, `waitpid`) | Shell cannot run commands without syscalls |
| PTY (pseudoterminal) | POSIX (`forkpty`, `posix_openpt`) | Shell ↔ terminal UI communication channel |

### 🟡 Good-to-Have (Quality — Significantly improves the product)

| Feature | Library | What It Adds |
|---------|---------|--------------|
| Text shaping & ligatures | **HarfBuzz** | Coding font ligatures (`->`, `!=`, `>=`), correct kerning |
| OpenGL function loading | **GLAD** | Access GL 3.3+ functions portably across drivers |
| Structured logging | **spdlog** + **fmt** | Debug/trace/error logging with levels and formatting |
| Unit testing | **GoogleTest** | Automated test suite for parser, tokenizer, builtins |
| Performance benchmarks | **Google Benchmark** | Measure parser throughput, render FPS, startup time |

### 🟢 Optional (Future — Can be added incrementally)

| Feature | Library | When Needed |
|---------|---------|-------------|
| Line editing (arrows, history, Ctrl-R) | **replxx** / **linenoise** | Sprint 3+ (interactive REPL improvements) |
| CLI argument parsing (`-c`, `--norc`) | **CLI11** | Sprint 2+ (shell startup flags) |
| Image backgrounds / wallpaper | **stb_image** | UI polish phase |
| Git branch/status in prompt | **libgit2** | Fancy prompt features |
| Full Unicode & emoji support | **ICU** / **utf8proc** | Internationalization phase |
| Config file parsing (`.shellrc`) | **toml++** / **yaml-cpp** | Settings & customization |
| Syntax highlighting | **tree-sitter** | Advanced editor features |

---

## Required Libraries (Must-Have)

### SDL2 — Windowing, Input & OpenGL Context

| | |
|---|---|
| **Purpose** | Cross-platform window creation, keyboard/mouse input, OpenGL context management. Auto-detects X11 or Wayland at runtime. |
| **Version** | 2.28+ recommended |
| **License** | zlib |
| **Source** | https://github.com/libsdl-org/SDL |
| **Docs** | https://wiki.libsdl.org/SDL2 |
| **API Reference** | https://wiki.libsdl.org/SDL2/CategoryAPI |

**Features we use from SDL2:**

| Feature | SDL2 API | Status |
|---------|----------|--------|
| Window creation | `SDL_CreateWindow()` | Must-have |
| OpenGL context | `SDL_GL_CreateContext()` | Must-have |
| Keyboard input | `SDL_KEYDOWN`, `SDL_TEXTINPUT` | Must-have |
| Mouse input (selection) | `SDL_MOUSEBUTTONDOWN`, `SDL_MOUSEMOTION` | Must-have |
| Window resize | `SDL_WINDOWEVENT_RESIZED` | Must-have |
| Clipboard | `SDL_GetClipboardText()`, `SDL_SetClipboardText()` | Good-to-have |
| HiDPI / scaling | `SDL_GL_GetDrawableSize()` | Good-to-have |
| Drag & drop | `SDL_DROPFILE` | Optional |
| Gamepad / joystick | — | Not needed (disabled) |
| Audio | — | Not needed (disabled) |

**Build flags for SDL2 (from source):**

```bash
cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DSDL_SHARED=ON \
    -DSDL_STATIC=OFF \
    -DSDL_TEST=OFF \
    -DSDL_AUDIO=OFF \          # not needed — saves binary size
    -DSDL_JOYSTICK=OFF \       # not needed
    -DSDL_HAPTIC=OFF \         # not needed
    -DSDL_SENSOR=OFF \         # not needed
    -DSDL_VIDEO=ON \           # required
    -DSDL_OPENGL=ON \          # required
    -DSDL_WAYLAND=ON \         # Wayland support
    -DSDL_X11=ON               # X11 support
```

#### Install

```bash
# Arch Linux
sudo pacman -S sdl2

# Ubuntu / Debian
sudo apt install libsdl2-dev

# Fedora
sudo dnf install SDL2-devel

# vcpkg
vcpkg install sdl2

# From source
git clone https://github.com/libsdl-org/SDL.git -b SDL2
cd SDL
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build
```

---

### OpenGL 3.3+ — GPU Rendering

| | |
|---|---|
| **Purpose** | Hardware-accelerated terminal cell grid rendering. Instanced drawing for entire screen in one draw call. |
| **Version** | 3.3 Core Profile (minimum) |
| **License** | System driver (Mesa / NVIDIA / AMD) |
| **Docs (Learn)** | https://learnopengl.com |
| **Docs (Reference)** | https://docs.gl |
| **Khronos Spec** | https://registry.khronos.org/OpenGL/specs/gl/glspec33.core.pdf |

**OpenGL features we use:**

| Feature | GL Function | Status |
|---------|-------------|--------|
| Vertex Array Objects | `glGenVertexArrays`, `glBindVertexArray` | Must-have |
| Vertex Buffer Objects | `glGenBuffers`, `glBufferData` | Must-have |
| Instanced rendering | `glDrawArraysInstanced` | Must-have |
| Texture atlases | `glTexImage2D`, `GL_RED` | Must-have |
| Shader programs | `glCreateShader`, `glLinkProgram` | Must-have |
| Blending (transparency) | `glEnable(GL_BLEND)` | Good-to-have |
| Framebuffer objects | `glGenFramebuffers` | Optional (for effects) |

#### Install

```bash
# Arch Linux
sudo pacman -S mesa

# Ubuntu / Debian
sudo apt install libgl-dev

# Fedora
sudo dnf install mesa-libGL-devel
```

> **GLAD**: You need an OpenGL loader to access GL 3.3+ functions.
> - Source: https://github.com/Dav1dde/glad
> - Generator: https://glad.dav1d.de (select: GL 3.3, Core Profile, C/C++)
> - License: MIT
> - Add the generated `glad.c` and `glad.h` to `third_party/glad/`

---

### FreeType — Font Rasterization

| | |
|---|---|
| **Purpose** | Loads font files (TTF, OTF) and rasterizes individual glyphs into bitmaps for the glyph atlas texture. |
| **Version** | 2.13+ recommended |
| **License** | FreeType License (BSD-like) or GPLv2 |
| **Source** | https://gitlab.freedesktop.org/freetype/freetype |
| **GitHub Mirror** | https://github.com/freetype/freetype |
| **Docs** | https://freetype.org/freetype2/docs/reference/index.html |
| **Tutorial** | https://freetype.org/freetype2/docs/tutorial/step1.html |

**Features we use from FreeType:**

| Feature | FreeType API | Status |
|---------|-------------|--------|
| Load font file | `FT_New_Face()` | Must-have |
| Set pixel size | `FT_Set_Pixel_Sizes()` | Must-have |
| Rasterize glyph | `FT_Load_Char()`, `FT_Render_Glyph()` | Must-have |
| Glyph metrics (advance, bearing) | `face->glyph->metrics` | Must-have |
| LCD subpixel rendering | `FT_RENDER_MODE_LCD` | Good-to-have |
| Font style (bold/italic) | `FT_STYLE_FLAG_BOLD` | Good-to-have |
| Variable fonts (weight axis) | `FT_Set_Var_Design_Coordinates()` | Optional |

**Build flags for FreeType (from source):**

```bash
cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DFT_DISABLE_BZIP2=ON \        # not needed — saves size
    -DFT_DISABLE_PNG=ON \          # not needed for terminal
    -DFT_DISABLE_BROTLI=OFF \      # keep — needed for WOFF2 fonts
    -DFT_DISABLE_HARFBUZZ=OFF      # keep — FreeType ↔ HarfBuzz integration
```

#### Install

```bash
# Arch Linux
sudo pacman -S freetype2

# Ubuntu / Debian
sudo apt install libfreetype-dev

# Fedora
sudo dnf install freetype-devel

# vcpkg
vcpkg install freetype

# From source
git clone https://gitlab.freedesktop.org/freetype/freetype.git
cd freetype
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build
```

---

### HarfBuzz — Text Shaping

| | |
|---|---|
| **Purpose** | Complex text shaping engine. Handles ligatures (e.g., `fi`, `->`, `!=` in coding fonts), kerning, and script-specific glyph selection. |
| **Version** | 8.0+ recommended |
| **License** | MIT (Old MIT) |
| **Source** | https://github.com/harfbuzz/harfbuzz |
| **Docs** | https://harfbuzz.github.io |
| **API Reference** | https://harfbuzz.github.io/harfbuzz-hb-buffer.html |
| **Getting Started** | https://harfbuzz.github.io/getting-started.html |

**Features we use from HarfBuzz:**

| Feature | HarfBuzz API | Status |
|---------|-------------|--------|
| Shape text run | `hb_shape()` | Good-to-have |
| Create buffer | `hb_buffer_create()` | Good-to-have |
| FreeType integration | `hb_ft_font_create()` | Good-to-have |
| Ligature substitution | GSUB table lookups (automatic) | Good-to-have |
| Kerning | GPOS table lookups (automatic) | Good-to-have |
| Script detection | `hb_buffer_guess_segment_properties()` | Optional |
| Fallback fonts | — (manual, not HarfBuzz's job) | Optional |

> **Note:** HarfBuzz is listed as "Good-to-have" rather than "Must-have" because
> a terminal can render without shaping — glyphs are monospaced and 1:1 mapped.
> However, for coding fonts with ligatures (Fira Code, JetBrains Mono, Cascadia Code),
> HarfBuzz is essential.

**Build flags for HarfBuzz (from source):**

```bash
meson setup build \
    -Dfreetype=enabled \           # FreeType integration
    -Dglib=disabled \              # not needed — saves size
    -Dgobject=disabled \           # not needed
    -Dcairo=disabled \             # not needed
    -Dicu=disabled \               # not needed (we use our own Unicode)
    -Dtests=disabled \             # skip tests for faster build
    -Ddocs=disabled                # skip docs
```

#### Install

```bash
# Arch Linux
sudo pacman -S harfbuzz

# Ubuntu / Debian
sudo apt install libharfbuzz-dev

# Fedora
sudo dnf install harfbuzz-devel

# vcpkg
vcpkg install harfbuzz

# From source
git clone https://github.com/harfbuzz/harfbuzz.git
cd harfbuzz
meson setup build
ninja -C build
sudo ninja -C build install
```

---

### libvterm — Terminal State Machine

| | |
|---|---|
| **Purpose** | Virtual terminal emulator library. Parses VT100/xterm escape sequences and maintains the terminal cell grid, cursor position, colors, and attributes. |
| **Version** | 0.3+ |
| **License** | MIT |
| **Source** | https://launchpad.net/libvterm |
| **GitHub Mirror** | https://github.com/neovim/libvterm |
| **Docs** | Inline in `vterm.h` header |
| **Used By** | Neovim (`:terminal`), pangoterm |

**Features we use from libvterm:**

| Feature | libvterm API | Status |
|---------|-------------|--------|
| Create virtual terminal | `vterm_new()` | Must-have |
| Feed input bytes | `vterm_input_write()` | Must-have |
| Read cell grid | `vterm_screen_get_cell()` | Must-have |
| Cursor position | `VTermState` callbacks | Must-have |
| 16-color ANSI | Automatic | Must-have |
| 256-color support | Automatic | Must-have |
| 24-bit truecolor | Automatic | Must-have |
| Bold / italic / underline / strikethrough | Cell attributes | Must-have |
| Terminal resize | `vterm_set_size()` | Must-have |
| Scrollback callbacks | `VTermScreenCallbacks.sb_pushline` | Good-to-have |
| Mouse tracking | DEC mode 1000/1002/1003 | Optional |
| Bracketed paste | DEC mode 2004 | Optional |
| OSC sequences (title, clipboard) | `VTermStateFallbacks` | Optional |

**Build flags for libvterm (from source):**

```bash
# libvterm uses Make, not CMake
make PREFIX=/usr/local
make install PREFIX=/usr/local

# Key defines (in Makefile):
#   -DINLINE_FONT=1     — embed the built-in font table
#   -DUNIT_TESTING=0    — skip test build
```

> **Note:** The Neovim fork at `github.com/neovim/libvterm` is more actively
> maintained than the original Launchpad source and is recommended.

#### Install

```bash
# Arch Linux
sudo pacman -S libvterm

# Ubuntu / Debian
sudo apt install libvterm-dev

# Fedora
sudo dnf install libvterm-devel

# vcpkg
vcpkg install libvterm

# From source (Neovim fork — recommended)
git clone https://github.com/neovim/libvterm.git
cd libvterm
make
sudo make install PREFIX=/usr/local
```

---

## Recommended Libraries (Good-to-Have)

### GLAD — OpenGL Function Loader

| | |
|---|---|
| **Purpose** | Loads OpenGL 3.3+ function pointers at runtime. Required because GL functions beyond 1.1 are not directly linkable on most platforms. |
| **License** | MIT |
| **Source** | https://github.com/Dav1dde/glad |
| **Generator** | https://glad.dav1d.de |
| **Status** | **Good-to-have** (technically must-have if targeting GL 3.3) |

**Generator settings:**
- Language: C/C++
- Specification: OpenGL
- Profile: Core
- API Version: 3.3
- Extensions: none needed initially

```bash
# Download generated files and place in third_party/glad/
# Or use glad2:
pip install glad2
glad --api gl:core=3.3 --out-path third_party/glad
```

---

### spdlog — Fast Logging

| | |
|---|---|
| **Purpose** | Structured, leveled logging (debug/info/warn/error) with minimal overhead. |
| **License** | MIT |
| **Source** | https://github.com/gabime/spdlog |
| **Docs** | https://github.com/gabime/spdlog/wiki |
| **Status** | **Good-to-have** |

**Build flags (when building from source):**

```bash
cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DSPDLOG_FMT_EXTERNAL=ON \      # use system fmt instead of bundled
    -DSPDLOG_BUILD_EXAMPLES=OFF \
    -DSPDLOG_BUILD_TESTS=OFF
```

```bash
# Arch
sudo pacman -S spdlog

# Ubuntu / Debian
sudo apt install libspdlog-dev

# vcpkg
vcpkg install spdlog
```

---

### fmt — Modern String Formatting

| | |
|---|---|
| **Purpose** | Type-safe `printf` replacement. Dependency of spdlog. Also useful standalone for error messages and debug output. |
| **License** | MIT |
| **Source** | https://github.com/fmtlib/fmt |
| **Docs** | https://fmt.dev/latest/index.html |
| **API Reference** | https://fmt.dev/latest/api.html |
| **Status** | **Good-to-have** |

```bash
# Arch
sudo pacman -S fmt

# Ubuntu / Debian
sudo apt install libfmt-dev

# vcpkg
vcpkg install fmt
```

---

### GoogleTest — Unit Testing

| | |
|---|---|
| **Purpose** | Unit test framework for testing parser, tokenizer, FD wrapper, builtins, and renderer components. |
| **License** | BSD-3-Clause |
| **Source** | https://github.com/google/googletest |
| **Docs** | https://google.github.io/googletest |
| **Primer** | https://google.github.io/googletest/primer.html |
| **Status** | **Good-to-have** |

```bash
# Arch
sudo pacman -S gtest

# Ubuntu / Debian
sudo apt install libgtest-dev

# vcpkg
vcpkg install gtest
```

---

### Google Benchmark — Performance Testing

| | |
|---|---|
| **Purpose** | Microbenchmark framework for measuring parser throughput, rendering FPS, and startup latency. |
| **License** | Apache-2.0 |
| **Source** | https://github.com/google/benchmark |
| **Docs** | https://github.com/google/benchmark/blob/main/docs/user_guide.md |
| **Status** | **Good-to-have** |

```bash
# Arch
sudo pacman -S benchmark

# Ubuntu / Debian
sudo apt install libbenchmark-dev

# vcpkg
vcpkg install benchmark
```

---

## Optional Libraries (Nice-to-Have)

| Library | Purpose | License | Source | When Needed |
|---------|---------|---------|--------|-------------|
| **replxx** | Line editing, history, Ctrl-R search | BSD-3 | https://github.com/AmokHuginnworksLLC/replxx | Sprint 3+ |
| **CLI11** | CLI argument parsing (`-c`, `--norc`) | BSD-3 | https://github.com/CLIUtils/CLI11 | Sprint 2+ |
| **stb_image** | Image loading (backgrounds, icons) | Public Domain | https://github.com/nothings/stb | UI polish |
| **libgit2** | Git branch/status in prompt | GPLv2 + linking exception | https://github.com/libgit2/libgit2 | Fancy prompt |
| **ICU** | Full Unicode (bidi, grapheme clusters) | Unicode License | https://github.com/unicode-org/icu | i18n phase |
| **utf8proc** | Lightweight UTF-8 processing | MIT | https://github.com/JuliaStrings/utf8proc | i18n phase |
| **toml++** | TOML config file (`.shellrc`) | MIT | https://github.com/marzer/tomlplusplus | Config phase |
| **tree-sitter** | Syntax highlighting in terminal | MIT | https://github.com/tree-sitter/tree-sitter | Advanced |

---

## Build Flags & CMake Options

### SHeLL CMake Options

| Flag | Default | Description |
|------|---------|-------------|
| `CMAKE_BUILD_TYPE` | `Debug` | Build type: `Debug`, `Release`, `RelWithDebInfo`, `MinSizeRel` |
| `ENABLE_SANITIZERS` | `ON` | Enable AddressSanitizer + UBSan in Debug builds |
| `BUILD_TESTING` | `OFF` | Build unit and integration tests (requires GoogleTest) |
| `BUILD_BENCHMARKS` | `OFF` | Build performance benchmarks (requires Google Benchmark) |

### Recommended CMake Presets

```bash
# Development (debug, sanitizers, fast compile)
cmake -B build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DENABLE_SANITIZERS=ON \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Release (optimized, no sanitizers, LTO)
cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_SANITIZERS=OFF \
    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON

# Release with debug info (profiling)
cmake -B build \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DENABLE_SANITIZERS=OFF

# Minimum size (embedded / resource-constrained)
cmake -B build \
    -DCMAKE_BUILD_TYPE=MinSizeRel \
    -DENABLE_SANITIZERS=OFF \
    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON

# With tests
cmake -B build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_TESTING=ON

# With benchmarks
cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_BENCHMARKS=ON
```

---

## Compiler Flags

### Applied by `CompilerWarnings.cmake`

#### GCC + Clang

| Flag | Purpose |
|------|---------|
| `-Wall` | Enable all standard warnings |
| `-Wextra` | Enable extra warnings beyond `-Wall` |
| `-Wpedantic` | Strict ISO C++ compliance warnings |
| `-Wshadow` | Warn on variable shadowing |
| `-Wnon-virtual-dtor` | Warn on classes with virtual methods but non-virtual destructor |
| `-Wold-style-cast` | Warn on C-style casts |
| `-Wcast-align` | Warn on potentially misaligned casts |
| `-Wconversion` | Warn on implicit type conversions |
| `-Wsign-conversion` | Warn on signed/unsigned conversion |
| `-Wdouble-promotion` | Warn on implicit float→double promotion |
| `-Wformat=2` | Enhanced format string checking |
| `-Wunused` | Warn on unused variables/functions |
| `-Woverloaded-virtual` | Warn on hidden virtual functions |
| `-Wnull-dereference` | Warn on null pointer dereference paths |

#### GCC Only (additional)

| Flag | Purpose |
|------|---------|
| `-Wmisleading-indentation` | Warn on misleading indentation |
| `-Wduplicated-cond` | Warn on duplicated if/else conditions |
| `-Wduplicated-branches` | Warn on identical if/else branches |
| `-Wlogical-op` | Warn on suspicious logical operations |
| `-Wuseless-cast` | Warn on unnecessary casts |

#### MSVC

| Flag | Purpose |
|------|---------|
| `/W4` | Warning level 4 (strict) |
| `/permissive-` | Strict standards conformance |

### Applied by `Sanitizers.cmake` (Debug builds only)

| Flag | Purpose |
|------|---------|
| `-fsanitize=address` | AddressSanitizer — detects buffer overflows, use-after-free, double-free |
| `-fsanitize=undefined` | UBSan — detects undefined behavior (signed overflow, null deref, etc.) |
| `-fno-omit-frame-pointer` | Preserve frame pointers for readable ASAN stack traces |
| `-fno-sanitize-recover=all` | Abort on first sanitizer error (don't continue) |

---

## Quick Install (All Required)

### Arch Linux
```bash
sudo pacman -S sdl2 mesa freetype2 harfbuzz libvterm
```

### Ubuntu / Debian
```bash
sudo apt install libsdl2-dev libgl-dev libfreetype-dev libharfbuzz-dev libvterm-dev
```

### Fedora
```bash
sudo dnf install SDL2-devel mesa-libGL-devel freetype-devel harfbuzz-devel libvterm-devel
```

### vcpkg (Cross-Platform)
```bash
vcpkg install sdl2 freetype harfbuzz libvterm
```

---

## Build Instructions

```bash
# 1. Clone the repo
git clone https://github.com/user/SHeLL.git
cd SHeLL

# 2. Install dependencies (pick your distro from above)

# 3. Configure (Debug build with sanitizers)
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# 4. Build
cmake --build build -j$(nproc)

# 5. Run
./build/bin/shell
```

---

## Rendering Backends (GPU Optional)

SHeLL is designed to run on **both machines with and without a GPU**.
The renderer auto-detects GPU availability at startup and selects the best backend.

### How It Works

```
Startup
  │
  ├─ Try: SDL_GL_CreateContext(OpenGL 3.3)
  │   ├─ Success (GPU or Mesa LLVMpipe) → Use OpenGL renderer
  │   └─ Fail → Fallback to SDL2 software renderer
  │
  └─ Result: Terminal works on any machine
```

### Backend Comparison

| Backend | When Used | How It Works | Performance | RAM |
|---------|-----------|-------------|-------------|-----|
| **OpenGL 3.3 (GPU)** | Discrete/integrated GPU present | Hardware-accelerated instanced rendering | ★★★★★ 60 FPS | ~10-14 MB |
| **OpenGL 3.3 (LLVMpipe)** | No GPU but Mesa installed | Mesa's software OpenGL on CPU | ★★★☆☆ 30-60 FPS | ~12-16 MB |
| **SDL2 Software** | No GPU, no Mesa GL support | SDL2 `SDL_Renderer` with `SDL_RENDERER_SOFTWARE` | ★★☆☆☆ 15-30 FPS | ~10-14 MB |

> **Key insight:** On Linux, Mesa's LLVMpipe automatically provides software OpenGL 3.3
> when no GPU is available (VMs, WSL, headless servers, old hardware). The same OpenGL
> code runs unchanged — just slower. This covers 95% of "no GPU" cases.
>
> The SDL2 software fallback is only needed for the remaining 5%: truly minimal systems
> with no Mesa, or environments where GL context creation fails entirely.

### Backend Details

#### 1. OpenGL 3.3 — GPU (Primary)

- Uses instanced rendering: entire terminal grid in **one draw call**
- Glyph atlas stored as GPU texture
- 60 FPS with near-zero CPU usage
- Works on any GPU from the last ~15 years (Intel HD 2000+, GTX 400+, Radeon HD 5000+)

#### 2. OpenGL 3.3 — Mesa LLVMpipe (Automatic Fallback)

- **No code changes** — same OpenGL 3.3 path, Mesa handles it in software
- Runs on CPU via LLVM JIT compilation of shaders
- Typical in VMs (VirtualBox, QEMU), WSL1, Docker containers, CI runners
- Install: `sudo pacman -S mesa` / `sudo apt install libgl1-mesa-dri`

#### 3. SDL2 Software Renderer (Last Resort Fallback)

- Uses `SDL_CreateRenderer()` with `SDL_RENDERER_SOFTWARE`
- FreeType rasterizes glyphs → blit to SDL surface → SDL copies to window
- No shaders, no GPU texture atlas
- Slightly different code path in renderer — uses `SDL_RenderCopy()` instead of GL calls
- Works everywhere SDL2 runs, even without Mesa

### Runtime Detection Logic

```cpp
// Pseudocode — actual implementation in src/renderer/gl_renderer.cpp

RendererBackend detectBackend(SDL_Window* window)
{
    // Attempt 1: OpenGL 3.3 Core
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GLContext gl = SDL_GL_CreateContext(window);
    if (gl)
    {
        // Works — could be real GPU or Mesa LLVMpipe, doesn't matter
        const char* renderer = (const char*)glGetString(GL_RENDERER);
        log::info("OpenGL renderer: {}", renderer);
        // renderer will be e.g. "NVIDIA GeForce RTX 3080" or "llvmpipe (LLVM 15.0)"
        return RendererBackend::OpenGL;
    }

    // Attempt 2: Fall back to SDL2 software
    log::warn("OpenGL 3.3 not available, falling back to software renderer");
    return RendererBackend::Software;
}
```

### Architecture: Renderer Interface

Both backends implement the same abstract interface so the rest of the code
doesn't care which one is active:

```cpp
// include/renderer/renderer.hpp
class IRenderer
{
public:
    virtual ~IRenderer() = default;

    virtual void init(int cols, int rows, float cellW, float cellH) = 0;
    virtual void resize(int cols, int rows) = 0;
    virtual void setCells(const CellGrid& grid) = 0;
    virtual void setCursor(int col, int row, CursorStyle style) = 0;
    virtual void render() = 0;
};

// src/renderer/gl_renderer.cpp   — implements IRenderer with OpenGL
// src/renderer/sw_renderer.cpp   — implements IRenderer with SDL2 software
```

---

## Dependency Graph

```
shell (executable)
├── shell_core (static lib)
│   └── no external deps — pure POSIX + C++ stdlib
│       Uses: fork(), execve(), waitpid(), pipe(), dup2(),
│             sigaction(), chdir(), getcwd(), read(), write()
│
├── terminal_core (static lib)
│   └── libvterm
│       Uses: vterm_new(), vterm_input_write(),
│             vterm_screen_get_cell(), vterm_set_size()
│
├── renderer_core (static lib)
│   ├── IRenderer interface (backend-agnostic)
│   │
│   ├── [GPU path] OpenGL 3.3 (via GLAD loader)
│   │   Uses: glDrawArraysInstanced(), glTexImage2D(),
│   │         glCreateShader(), glBufferData()
│   │
│   ├── [CPU path] SDL2 Software Renderer
│   │   Uses: SDL_CreateRenderer(SOFTWARE), SDL_RenderCopy(),
│   │         SDL_CreateTextureFromSurface()
│   │
│   ├── FreeType (used by both paths)
│   │   Uses: FT_New_Face(), FT_Load_Char(), FT_Render_Glyph()
│   │
│   └── HarfBuzz (used by both paths)
│       Uses: hb_shape(), hb_buffer_create(), hb_ft_font_create()
│
└── SDL2
    Uses: SDL_CreateWindow(), SDL_GL_CreateContext() [GPU]
          SDL_CreateRenderer() [CPU], SDL_PollEvent()
```
