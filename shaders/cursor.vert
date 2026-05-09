// =========================================================
// SHeLL — cursor.vert
// =========================================================
// Vertex shader for the terminal cursor.
// Renders a quad at the current cursor position.
// =========================================================

#version 330 core

layout (location = 0) in vec2 aPos;

uniform vec2 uCellSize;
uniform vec2 uCursorPos;      // cursor position (col, row)
uniform vec2 uViewportSize;

void main()
{
    vec2 pos = aPos * uCellSize + uCursorPos * uCellSize;
    vec2 ndc = (pos / uViewportSize) * 2.0 - 1.0;
    ndc.y = -ndc.y;

    gl_Position = vec4(ndc, 0.0, 1.0);
}
