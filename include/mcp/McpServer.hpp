// =========================================================
// SHeLL — mcp/McpServer.hpp
// =========================================================
// MCP Server implementation.
// Listens for JSON-RPC 2.0 requests over stdio (or socket)
// and dispatches them to registered tool handlers.
//
// Used when SHeLL runs as: shell --mcp-server
// AI assistants (Claude, Cursor) connect and call tools like
// execute_command, read_file, list_directory, etc.
// =========================================================

#pragma once
