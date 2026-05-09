# SHeLL — MCP Architecture & Data Flow

## Overview

SHeLL supports MCP (Model Context Protocol) in **two modes** + an **AI-assist** feature:

| Mode | Direction | Example |
|------|-----------|---------|
| **MCP Server** | AI → Shell | Claude Desktop calls `execute_command("ls")` on your shell |
| **MCP Client** | Shell → AI/Tools | User types `mcp call db query "SELECT *"` |
| **AI Assist** | User ↔ AI ↔ Shell | User types `?? find large files` → AI generates command |

---

## Mode 1: MCP Server (AI Controls the Shell)

> SHeLL runs as an MCP server. An AI assistant (Claude, Cursor, etc.)
> sends JSON-RPC requests over stdio, and SHeLL executes them.

### Full Data Flow

```
  AI Assistant (Claude/Cursor)
       │
       │ ① JSON-RPC request over stdio
       │    {"jsonrpc":"2.0","method":"tools/call",
       │     "params":{"name":"execute_command",
       │               "arguments":{"command":"ls -la /tmp"}},
       │     "id":1}
       │
       ▼
  ┌─────────────────────────────┐
  │  MCP Transport (stdio)      │  ← reads from stdin
  │  src/mcp/McpTransport.cpp   │
  └──────────────┬──────────────┘
                 │
                 │ ② Raw string → Json::parse()
                 ▼
  ┌─────────────────────────────┐
  │  Json Wrapper               │  ← RapidJSON under the hood
  │  include/utils/Json.hpp     │
  │                             │
  │  Json msg = Json::parse(raw);
  │  string method = msg["method"];
  │  Json params = msg["params"];
  └──────────────┬──────────────┘
                 │
                 │ ③ Parsed request → McpServer dispatch
                 ▼
  ┌─────────────────────────────┐
  │  MCP Server                 │
  │  src/mcp/McpServer.cpp      │
  │                             │
  │  Routes "tools/call" →      │
  │  looks up tool by name      │
  │  → calls McpTools handler   │
  └──────────────┬──────────────┘
                 │
                 │ ④ Tool name = "execute_command"
                 │   Arguments = {"command": "ls -la /tmp"}
                 ▼
  ┌─────────────────────────────┐
  │  MCP Tools                  │
  │  src/mcp/McpTools.cpp       │
  │                             │
  │  execute_command():         │
  │    1. Tokenize "ls -la /tmp"│
  │    2. fork()                │
  │    3. execvp("ls", args)    │
  │    4. Capture stdout/stderr │
  │    5. waitpid() → exit code │
  └──────────────┬──────────────┘
                 │
                 │ ⑤ Command output:
                 │   stdout = "drwxr-xr-x 2 user ..."
                 │   exit_code = 0
                 ▼
  ┌─────────────────────────────┐
  │  Json Wrapper (build)       │
  │                             │
  │  Json response = Json::object();
  │  response.set("jsonrpc", "2.0");
  │  response.set("id", requestId);
  │  response.set("result", {    │
  │    "content": [{             │
  │      "type": "text",         │
  │      "text": "drwxr-xr-x.." │
  │    }]                        │
  │  });                         │
  │  std::string out = response.dump();
  └──────────────┬──────────────┘
                 │
                 │ ⑥ JSON-RPC response over stdio
                 │   {"jsonrpc":"2.0","id":1,
                 │    "result":{"content":[{"type":"text",
                 │    "text":"drwxr-xr-x 2 user ..."}]}}
                 ▼
  ┌─────────────────────────────┐
  │  MCP Transport (stdio)      │  ← writes to stdout
  │  src/mcp/McpTransport.cpp   │
  └──────────────┬──────────────┘
                 │
                 ▼
  AI Assistant receives output
  and shows it to the user
```

### Server Startup

```bash
# AI assistant launches SHeLL in MCP server mode:
$ shell --mcp-server

# Or via MCP config (claude_desktop_config.json):
{
  "mcpServers": {
    "shell": {
      "command": "/usr/local/bin/shell",
      "args": ["--mcp-server"]
    }
  }
}
```

---

## Mode 2: MCP Client (Shell Uses External Tools)

> The user connects to external MCP servers from within SHeLL
> and invokes their tools as shell commands.

### Full Data Flow

```
  User types in terminal
       │
       │ ① "mcp call sqlite query SELECT * FROM users"
       │
       ▼
  ┌─────────────────────────────┐
  │  REPL + Parser              │
  │  src/shell/Repl.cpp         │
  │                             │
  │  Detects "mcp" builtin      │
  │  Parses: server = "sqlite"  │
  │          tool   = "query"   │
  │          args   = "SELECT…" │
  └──────────────┬──────────────┘
                 │
                 │ ② Build JSON-RPC request
                 ▼
  ┌─────────────────────────────┐
  │  Json Wrapper (build)       │
  │                             │
  │  Json req = Json::object(); │
  │  req.set("jsonrpc", "2.0");│
  │  req.set("method",         │
  │          "tools/call");     │
  │  req.set("params", {       │
  │    "name": "query",        │
  │    "arguments": {          │
  │      "sql": "SELECT * …"   │
  │    }                       │
  │  });                       │
  │  req.set("id", nextId());  │
  └──────────────┬──────────────┘
                 │
                 │ ③ Send over transport (stdio pipe to server process)
                 ▼
  ┌─────────────────────────────┐
  │  MCP Client                 │
  │  src/mcp/McpClient.cpp      │
  │                             │
  │  Manages connections to     │
  │  external MCP server        │
  │  processes                  │
  │                             │
  │  write(serverStdin, json)   │
  │  read(serverStdout) → resp  │
  └──────────────┬──────────────┘
                 │
                 │ ④ JSON-RPC to external MCP server
                 ▼
  ┌─────────────────────────────┐
  │  External MCP Server        │
  │  (e.g. sqlite-mcp-server)   │
  │                             │
  │  Receives request           │
  │  Executes SQL query         │
  │  Returns result             │
  └──────────────┬──────────────┘
                 │
                 │ ⑤ JSON-RPC response
                 │   {"jsonrpc":"2.0","id":1,
                 │    "result":{"content":[{"type":"text",
                 │    "text":"id|name|email\n1|Alice|..."}]}}
                 ▼
  ┌─────────────────────────────┐
  │  Json Wrapper (parse)       │
  │                             │
  │  Json resp = Json::parse(); │
  │  string text =              │
  │    resp["result"]           │
  │        ["content"][0]       │
  │        ["text"];            │
  └──────────────┬──────────────┘
                 │
                 │ ⑥ Display result in terminal
                 ▼
  ┌─────────────────────────────┐
  │  Terminal Display           │
  │                             │
  │  mysh [0]> mcp call sqlite  │
  │  query "SELECT * FROM users"│
  │                             │
  │  id | name  | email         │
  │  1  | Alice | alice@ex.com  │
  │  2  | Bob   | bob@ex.com    │
  │                             │
  │  mysh [0]>                  │
  └─────────────────────────────┘
```

---

## Mode 3: AI-Assisted Terminal

> User asks a natural language question, SHeLL sends it to an
> AI MCP server, gets back a shell command, and offers to run it.

### Flow

```
  User types
       │
       │ ① "?? find all cpp files larger than 1MB modified this week"
       │
       ▼
  ┌─────────────────────────────┐
  │  REPL                       │
  │  Detects "??" prefix        │
  │  Strips prefix → NL query   │
  └──────────────┬──────────────┘
                 │
                 │ ② Build MCP request with context
                 ▼
  ┌─────────────────────────────┐
  │  MCP Client → AI Server     │
  │                             │
  │  Json req:                  │
  │  { "method": "tools/call",  │
  │    "params": {              │
  │      "name": "suggest_cmd", │
  │      "arguments": {         │
  │        "query": "find all   │
  │          cpp files …",      │
  │        "cwd": "/home/user", │
  │        "shell": "mysh",     │
  │        "os": "linux"        │
  │      }                      │
  │    }                        │
  │  }                          │
  └──────────────┬──────────────┘
                 │
                 │ ③ AI server responds
                 ▼
  ┌─────────────────────────────┐
  │  AI Response                │
  │                             │
  │  "result": {                │
  │    "command": "find . -name │
  │      '*.cpp' -size +1M     │
  │      -mtime -7",           │
  │    "explanation": "Searches │
  │      recursively for .cpp  │
  │      files over 1MB…"      │
  │  }                         │
  └──────────────┬──────────────┘
                 │
                 │ ④ Show suggestion to user
                 ▼
  ┌─────────────────────────────┐
  │  Terminal Display           │
  │                             │
  │  ┌─ AI Suggestion ────────┐ │
  │  │                        │ │
  │  │  find . -name '*.cpp'  │ │
  │  │    -size +1M -mtime -7 │ │
  │  │                        │ │
  │  │  Searches recursively  │ │
  │  │  for .cpp files over   │ │
  │  │  1MB modified in the   │ │
  │  │  last 7 days.          │ │
  │  │                        │ │
  │  │  [Enter] Run           │ │
  │  │  [e] Edit  [q] Cancel  │ │
  │  └────────────────────────┘ │
  └──────────────┬──────────────┘
                 │
                 │ ⑤ User presses Enter
                 ▼
  ┌─────────────────────────────┐
  │  Executor                   │
  │  fork() + execvp()          │
  │  Runs the suggested command │
  └─────────────────────────────┘
```

---

## Where the Json Wrapper Fits

The wrapper is the **serialization/deserialization layer** used by all MCP components:

```
                    ┌──────────────┐
                    │  Json.hpp    │
                    │  (Wrapper)   │
                    └──────┬───────┘
                           │
          ┌────────────────┼────────────────┐
          │                │                │
    ┌─────▼─────┐   ┌─────▼─────┐   ┌─────▼─────┐
    │ McpServer │   │ McpClient │   │ McpTools  │
    │           │   │           │   │           │
    │ parse req │   │ build req │   │ build     │
    │ build resp│   │ parse resp│   │ result    │
    └───────────┘   └───────────┘   └───────────┘
```

**Read path (deserialize):**
```
stdin bytes → Json::parse(raw) → msg["method"], msg["params"] → dispatch
```

**Write path (serialize):**
```
result data → Json::object().set("result", ...) → json.dump() → stdout bytes
```

---

## Transport Options

| Transport | How It Works | When Used |
|-----------|-------------|-----------|
| **stdio** | Read stdin, write stdout (JSON-RPC per line) | Default. Used by Claude Desktop, Cursor. |
| **Unix socket** | `/tmp/shell-mcp.sock` | Multiple clients, persistent server. |
| **SSE (HTTP)** | HTTP Server-Sent Events | Remote/web-based AI clients. |

For SHeLL, **stdio is primary** (zero dependencies, matches how the shell already works).

---

## Security

| Concern | Mitigation |
|---------|-----------|
| AI runs `rm -rf /` | Tool allowlist + confirmation prompt for destructive commands |
| Arbitrary file access | Configurable working directory sandbox |
| Secrets in env vars | Filter `PASSWORD`, `TOKEN`, `SECRET` from `get_environment` |
| Untrusted MCP servers | User must explicitly `mcp connect` — no auto-discovery |
