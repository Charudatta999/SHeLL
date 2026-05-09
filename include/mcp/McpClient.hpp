// =========================================================
// SHeLL — mcp/McpClient.hpp
// =========================================================
// MCP Client implementation.
// Connects to external MCP server processes (over stdio pipe)
// and invokes their tools on behalf of the user.
//
// Usage from shell:
//   mcp connect sqlite
//   mcp tools
//   mcp call sqlite query "SELECT * FROM users"
// =========================================================

#pragma once
