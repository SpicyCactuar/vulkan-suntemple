#version 460

const float alphaThreshold = 0.5f;

layout(set = 1, binding = 4) uniform sampler2D alphaMask;

layout(location = 0) in vec2 uv;

void main() {
    // Discard fragments with alpha below a threshold
    float transparency = texture(alphaMask, uv).a;
    if (transparency < alphaThreshold) {
        discard; // Don't write to depth buffer and terminate processing
    }

    // Writes to depth buffer automatically
}
