# MYSH — Roadmap

A syscall-first Linux shell implementation focused on:

- process architecture
- shell semantics
- Unix internals
- robust systems programming
- observability-driven debugging

---

# Vision

Build a production-grade educational shell that teaches:

- Linux process lifecycle
- file descriptor ownership
- signal orchestration
- process groups
- shell parsing
- async runtime design
- event-driven systems

The project intentionally prioritizes:

- correctness
- syscall understanding
- architecture clarity
- debuggability
- robustness

over feature velocity.

---

# Core Engineering Principles

## 1. Syscall First

Understand and directly model:

- fork()
- execve()
- waitpid()
- pipe()
- dup2()
- sigaction()
- kill()
- setsid()
- tcsetpgrp()

Avoid abstraction leakage.

---

## 2. Explicit Ownership

Every resource must have a clear owner:

- file descriptors
- processes
- pipes
- signal handlers
- child lifecycle

No hidden ownership.

---

## 3. Observability First

All behavior must be verifiable with:

- strace
- ltrace
- valgrind
- ASAN
- UBSAN
- gdb

If behavior cannot be debugged easily,
the architecture is wrong.

---

## 4. Linux-Native Architecture

The shell should behave like a real Unix process:

- signal aware
- terminal aware
- process-group aware
- fd aware

---

# High Level Architecture

```text
REPL
  ↓
Tokenizer
  ↓
Parser
  ↓
Command Model
  ↓
Builtin Dispatcher / Executor
  ↓
fork()
  ├── Parent Path
  │     └── waitpid()
  │
  └── Child Path
        └── execvp()
