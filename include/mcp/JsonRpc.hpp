// =========================================================
// SHeLL — mcp/JsonRpc.hpp
// =========================================================
// JSON-RPC 2.0 message helpers.
// Provides typed request/response/error/notification builders
// on top of the Json wrapper.
//
//   JsonRpc::request("tools/call", params, id)
//   JsonRpc::response(id, result)
//   JsonRpc::error(id, code, message)
//   JsonRpc::notification("notifications/progress", data)
// =========================================================

#pragma once
