// =========================================================
// SHeLL — cell.frag
// =========================================================
// Fragment shader for terminal cell rendering.
// Samples the glyph atlas texture and composites fg/bg colors.
// =========================================================

#version 330 core

in vec2 vTexCoord;
in vec4 vFgColor;
in vec4 vBgColor;

uniform sampler2D uGlyphAtlas;

out vec4 fragColor;

void main()
{
    // Sample glyph alpha from the atlas (single-channel)
    float glyphAlpha = texture(uGlyphAtlas, vTexCoord).r;

    // Blend foreground glyph over background color
    fragColor = mix(vBgColor, vFgColor, glyphAlpha);
}
