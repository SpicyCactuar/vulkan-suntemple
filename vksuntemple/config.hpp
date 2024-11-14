#pragma once

#if !defined(GLM_FORCE_RADIANS)
#	define GLM_FORCE_RADIANS
#endif

#include <chrono>
#include <vulkan/vulkan_core.h>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include "../vkutils/angle.hpp"

using namespace vkutils::literals;

namespace cfg {
    // Compiled shader code for the graphics pipeline(s)
    // See sources in vksuntemple/shaders/*.
#   define SHADERDIR_ "assets/vksuntemple/shaders/"
    constexpr const char* opaqueShadowVertPath = SHADERDIR_ "opaque_shadow_map.vert.spv";
    constexpr const char* opaqueShadowFragPath = SHADERDIR_ "opaque_shadow_map.frag.spv";
    constexpr const char* alphaShadowVertPath = SHADERDIR_ "alpha_shadow_map.vert.spv";
    constexpr const char* alphaShadowFragPath = SHADERDIR_ "alpha_shadow_map.frag.spv";
    constexpr const char* offscreenVertPath = SHADERDIR_ "offscreen.vert.spv";
    constexpr const char* offscreenOpaqueFragPath = SHADERDIR_ "offscreen_opaque.frag.spv";
    constexpr const char* offscreenAlphaFragPath = SHADERDIR_ "offscreen_alpha.frag.spv";
    constexpr const char* fullscreenVertPath = SHADERDIR_ "fullscreen.vert.spv";
    constexpr const char* fullscreenFragPath = SHADERDIR_ "fullscreen.frag.spv";
#	undef SHADERDIR_

    // Models
#	define MODELDIR_ "assets/"
    constexpr char const* sunTempleObjZstdPath = MODELDIR_ "suntemple.spicymesh";
#	undef MODELDIR_

    constexpr unsigned int windowWidth = 1600;
    constexpr unsigned int windowHeight = 1000;

    constexpr glm::vec3 cameraInitialPosition = {0.0f, 6.0f, 8.0f};
    const glm::mat4 cameraInitialRotation = glm::rotate(glm::radians(-15.0f), glm::vec3(1.0f, 0.0f, 0.0f)) *
                                            glm::rotate(glm::radians(-5.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    constexpr float cameraNear = 0.1f;
    constexpr float cameraFar = 100.0f;

    constexpr auto cameraFov = 60.0_degf;

    using Clock = std::chrono::steady_clock;
    using Secondsf = std::chrono::duration<float, std::ratio<1>>;

    constexpr float cameraBaseSpeed = 1.7f; // units/second
    constexpr float cameraFastMult = 5.0f; // speed multiplier
    constexpr float cameraSlowMult = 0.05f; // speed multiplier

    constexpr float cameraMouseSensitivity = 0.001f; // radians per pixel

    constexpr VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
    constexpr VkFormat offscreenFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

    constexpr glm::vec3 ambient = {0.02f, 0.02f, 0.02f};

    constexpr glm::vec3 lightColour = {1.0f, 1.0f, 1.0f};
    constexpr glm::vec3 lightPosition = {-0.2972f, 7.3100f, 11.9532f};
    // Look forward, and slightly downward to avoid shadow acne
    constexpr glm::vec3 lightLookCenter = lightPosition + glm::vec3{0.0f, -0.01f, -1.0f};
    constexpr float lightNear = 1.0f;
    constexpr float lightFar = 100.0f;
    constexpr auto lightFov = 90.0_degf;

    constexpr VkExtent2D shadowMapExtent = {.width = 2048, .height = 2048};

    // Bias matrix to transform coordinates from [-1, 1] to [0, 1]
    // Only (x, y) is shifted and scaled
    // textureProj uses position_lcs.zw as-is for depth comparison and perspective divide respectively
    constexpr glm::mat4 shadowTransformationMatrix = {
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f
    };
}
