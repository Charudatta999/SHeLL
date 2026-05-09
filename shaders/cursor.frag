// =========================================================
// SHeLL — cursor.frag
// =========================================================
// Fragment shader for the terminal cursor.
// Supports blinking via a uniform alpha value.
// =========================================================

#version 330 core

uniform vec4 uCursorColor;
uniform float uBlinkAlpha;    // 0.0 = invisible, 1.0 = fully visible

out vec4 fragColor;

void main()
{
    fragColor = vec4(uCursorColor.rgb, uCursorColor.a * uBlinkAlpha);
}
