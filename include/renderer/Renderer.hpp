// =========================================================
// SHeLL — renderer/renderer.hpp
// =========================================================
// Abstract renderer interface.
// Both the OpenGL (GPU) and SDL2 software (CPU) backends
// implement this interface, allowing runtime backend selection.
//
// The rest of the application code uses only IRenderer,
// so it works identically regardless of GPU availability.
// =========================================================

#pragma once
