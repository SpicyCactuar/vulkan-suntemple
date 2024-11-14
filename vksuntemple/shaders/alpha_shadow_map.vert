#version 460

layout(std140, set = 0, binding = 0) uniform Scene {
    mat4 VP;
    mat4 LP;
    mat4 SLP;
} scene;

layout(location = 0) in vec3 vertexPosition_wcs;
layout(location = 1) in vec2 vertexUV;

layout(location = 0) out vec2 uv;

void main() {
    gl_Position = scene.LP * vec4(vertexPosition_wcs, 1.0f);
    uv = vertexUV;
}
