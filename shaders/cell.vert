// =========================================================
// SHeLL — cell.vert
// =========================================================
// Vertex shader for instanced terminal cell grid rendering.
// Each instance represents one cell in the terminal grid.
// =========================================================

#version 330 core

layout (location = 0) in vec2 aPos;       // quad vertex position
layout (location = 1) in vec2 aTexCoord;   // glyph atlas UV

// Per-instance attributes
layout (location = 2) in vec2 aCellPos;    // cell grid position (col, row)
layout (location = 3) in vec4 aFgColor;    // foreground color (RGBA)
layout (location = 4) in vec4 aBgColor;    // background color (RGBA)
layout (location = 5) in vec4 aGlyphUV;    // glyph UV rect in atlas (u0, v0, u1, v1)

uniform vec2 uCellSize;       // cell dimensions in pixels
uniform vec2 uViewportSize;   // window dimensions in pixels

out vec2 vTexCoord;
out vec4 vFgColor;
out vec4 vBgColor;

void main()
{
    // Scale quad to cell size and offset by grid position
    vec2 pos = aPos * uCellSize + aCellPos * uCellSize;

    // Convert from pixel coords to NDC (-1 to 1)
    vec2 ndc = (pos / uViewportSize) * 2.0 - 1.0;
    ndc.y = -ndc.y; // flip Y (screen coords: top-left origin)

    gl_Position = vec4(ndc, 0.0, 1.0);

    // Interpolate glyph atlas UV
    vTexCoord = mix(aGlyphUV.xy, aGlyphUV.zw, aTexCoord);
    vFgColor = aFgColor;
    vBgColor = aBgColor;
}
