// =========================================================
// SHeLL — utils/Json.hpp
// =========================================================
// Thin wrapper over RapidJSON providing nlohmann-like ergonomics.
// Zero-copy parsing, minimal allocations, clean API.
//
// Usage:
//   Json msg = Json::parse(raw);
//   std::string_view method = msg["method"];
//   int id = msg["id"];
//
//   Json resp = Json::object();
//   resp.set("jsonrpc", "2.0");
//   resp.set("id", 1);
//   std::string out = resp.dump();
// =========================================================

#pragma once
