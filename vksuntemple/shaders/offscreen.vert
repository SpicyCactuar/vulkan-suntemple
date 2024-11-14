#version 460

layout(std140, set = 0, binding = 0) uniform Scene {
    mat4 VP;
    mat4 LP;
    mat4 SLP;
} scene;

layout(location = 0) in vec3 vertexPosition_wcs;
layout(location = 1) in vec2 vertexUV;
layout(location = 2) in vec3 vertexNormal_wcs;
layout(location = 3) in vec4 vertexTangent;

layout(location = 0) out vec3 position_wcs;
layout(location = 1) out vec2 uv;
layout(location = 2) out vec3 normal_wcs;
layout(location = 3) out mat3 TBN;
layout(location = 6) out vec4 position_lcs;

void main() {
    gl_Position = scene.VP * vec4(vertexPosition_wcs, 1.0f);
    position_wcs = vertexPosition_wcs;
    uv = vertexUV;
    normal_wcs = vertexNormal_wcs;
    vec3 vertexBitangent = vertexTangent.w * cross(vertexNormal_wcs, vertexTangent.xyz);
    TBN = mat3(vertexTangent.xyz, vertexBitangent, vertexNormal_wcs);
    position_lcs = scene.SLP * vec4(vertexPosition_wcs, 1.0f);
}
