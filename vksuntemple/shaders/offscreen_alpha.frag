#version 460

const float PI = 3.14159265359f;
const float EPS = 0.0000001f;
const vec3 dielectricReflectivity = vec3(0.04f);
const float alphaThreshold = 0.5f;
const vec3 noShadows = vec3(1.0f);

// See state::VisalusationMode for specification
const uint pbrMode = 1;
const uint normalMode = 2;
const uint viewDirectionMode = 3;
const uint lightDirectionMode = 4;
const uint roughnessMode = 5;
const uint metalnessMode = 6;
const uint normalMapMode = 7;
const uint baseMode = 8;

// See state::PBRTerm for specification
const uint allTerms = 1;
const uint ambientTerm = 2;
const uint diffuseTerm = 3;
const uint distributionTerm = 4;
const uint fresnelTerm = 5;
const uint geometryTerm = 6;
const uint specularTerm = 7;

// See state::ShadingDetails for specification
const uint normalMapping = 0x01;
const uint shadows = 0x02;
const uint pcf = 0x04;

struct PointLight {
    vec3 position_wcs;
    vec3 colour;
};

layout (std140, set = 1, binding = 0) uniform Shade {
    uint visualisationMode;
    uint pbrTerm;
    uint detailsMask;
    vec3 cameraPosition_wcs;
    vec3 ambient;
    PointLight light;
} shade;

layout (set = 1, binding = 1) uniform sampler2DShadow shadow;

layout (set = 2, binding = 0) uniform sampler2D baseColour;
layout (set = 2, binding = 1) uniform sampler2D roughness;
layout (set = 2, binding = 2) uniform sampler2D metalness;
layout (set = 2, binding = 3) uniform sampler2D normalMap;
layout (set = 2, binding = 4) uniform sampler2D alphaMask;

layout (push_constant) uniform MeshPushConstants {
    vec3 colour;
} mesh;

layout (location = 0) in vec3 position_wcs;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec3 normal_wcs;
layout (location = 3) in mat3 TBN;
layout (location = 6) in vec4 position_lcs;

layout (location = 0) out vec4 colour;

float saturate(float value) {
    return clamp(value, 0.0f, 1.0f);
}

float pcfShadowFactor() {
    // Average shadow over 9-tiled area
    float shadowFactor = 0.0f;
    vec2 texelSize = 1.0f / textureSize(shadow, 0);

    for (int x = -7; x <= 7; ++x) {
        for (int y = -7; y <= 7; ++y) {
            vec4 samplePosition = vec4(position_lcs.xy + vec2(x, y) * texelSize, position_lcs.z, position_lcs.w);
            shadowFactor += textureProj(shadow, samplePosition);
        }
    }

    return shadowFactor / 225.0f;
}

vec3 shadowModulator() {
    if ((shade.detailsMask & shadows) == 0) {
        return noShadows;
    }

    float shadowFactor = 1.0f;
    if ((shade.detailsMask & pcf) != 0) {
        shadowFactor = pcfShadowFactor();
    } else {
        shadowFactor = textureProj(shadow, position_lcs);
    }

    return vec3(shadowFactor);
}

vec3 fresnelSchlick(float M, vec3 cMat, float cosTheta) {
    vec3 f0 = mix(dielectricReflectivity, cMat, M);

    return f0 + (1.0f - f0) * pow(1.0f - cosTheta, 5.0f);
}

vec3 diffuseColour(float M, vec3 cMat, float cosTheta) {
    vec3 F = fresnelSchlick(M, cMat, cosTheta);

    return (cMat / PI) * (vec3(1.0f) - F) * (1.0f - M);
}

float beckmannDistribution(float alpha, float nh) {
    float alphaSquared = alpha * alpha;
    float nhSquared = nh * nh;
    float numerator = exp((nhSquared - 1.0f) / max(EPS, alphaSquared * nhSquared));
    float denominator = max(EPS, PI * alphaSquared * nhSquared * nhSquared);

    return numerator / denominator;
}

float cookTorranceMask(float nh, float nv, float nl, float vh) {
    return min(1.0f, 2.0f * min(nh * nv / vh, nh * nl / vh));
}

vec3 pbrColour(vec3 fragNormal_wcs) {
    // Parameters
    float roughness = texture(roughness, uv).r;
    float alpha = roughness * roughness;
    float M = texture(metalness, uv).r;
    vec3 cMat = texture(baseColour, uv).rgb;
    vec3 cLight = shade.light.colour;
    vec3 cAmbient = shade.ambient;
    vec3 n = normalize(fragNormal_wcs);
    vec3 l = normalize(shade.light.position_wcs - position_wcs);
    vec3 v = normalize(shade.cameraPosition_wcs - position_wcs);
    vec3 h = normalize(l + v);
    float nl = saturate(dot(n, l));
    float nv = saturate(dot(n, v));
    float nh = saturate(dot(n, h));
    float vh = saturate(dot(v, h));
    float lh = saturate(dot(l, h));

    // Ambient
    vec3 ambient = shade.ambient * cMat;

    // Diffuse
    vec3 diffuse = diffuseColour(M, cMat, vh);

    // Normal Distribution term
    float D = beckmannDistribution(alpha, nh);

    // Fresnel term
    vec3 F = fresnelSchlick(M, cMat, lh);

    // Masking term
    float G = cookTorranceMask(nh, nv, nl, vh);

    // Specular
    float denominator = max(EPS, 4 * nv * nl);
    vec3 specular = (D * F * G) / denominator;

    // BRDF
    vec3 brdf = diffuse + specular;

    switch (shade.pbrTerm) {
        case ambientTerm:
            return ambient;
        case diffuseTerm:
            return diffuse;
        case distributionTerm:
            return vec3(D);
        case fresnelTerm:
            return vec3(F);
        case geometryTerm:
            return vec3(G);
        case specularTerm:
            return specular;
        default:
    // allTermsF => full equation
            return ambient + shadowModulator() * cLight * nl * brdf;
    }
}

void main() {
    float transparency = texture(alphaMask, uv).a;
    if (transparency < alphaThreshold) {
        discard;
    }

    // Re-orient normal if Normal Mapping is enabled
    bool normalMappingEnabled = (shade.detailsMask & normalMapping) != 0;
    vec3 fragNormal_wcs = normal_wcs;
    if (normalMappingEnabled) {
        fragNormal_wcs = TBN * (texture(normalMap, uv).rgb * 2.0 - 1.0);
    }

    vec3 fragColour;
    switch (shade.visualisationMode) {
        case pbrMode:
            fragColour = pbrColour(fragNormal_wcs);
            break;
        case normalMode:
            fragColour = fragNormal_wcs;
            break;
        case viewDirectionMode:
    // Fragment -> Camera
            fragColour = normalize(shade.cameraPosition_wcs - position_wcs);
            break;
        case lightDirectionMode:
    // Fragment -> Light
            fragColour = normalize(shade.light.position_wcs - position_wcs);
            break;
        case roughnessMode:
    // Expands to greyscale colour [r, r, r]
            fragColour = texture(roughness, uv).rgb;
            break;
        case metalnessMode:
    // Expands to greyscale colour [m, m, m]
            fragColour = texture(metalness, uv).rgb;
            break;
        case normalMapMode:
            fragColour = texture(normalMap, uv).rgb;
            break;
        case baseMode:
            fragColour = texture(baseColour, uv).rgb * mesh.colour;
            break;
        default:
            break;
    }

    colour = vec4(fragColour, 1.0f);
}