#version 460 core

layout(set = 0, binding = 0) uniform sampler2D screen;

layout(std140, set = 0, binding = 1) uniform ScreenEffects {
    bool toneMappingEnabled;
} screenEffects;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 colour;

void main() {
    if (screenEffects.toneMappingEnabled) {
        // Sample HDR screen texture
        vec4 hdrColour = texture(screen, uv);

        // Apply Reinhard tone mapping operator
        vec3 sdrColour = hdrColour.rgb / (1.0 + hdrColour.rgb);

        // Output the tone-mapped color
        colour = vec4(sdrColour, 1.0);
    } else {
        colour = texture(screen, uv);
    }
}