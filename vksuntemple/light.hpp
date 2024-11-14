#pragma once

#include <glm/glm.hpp>

namespace glsl {
    struct PointLightUniform {
        glm::vec4 position;
        glm::vec4 colour;
    };

    static_assert(sizeof(PointLightUniform) <= 65536,
                  "PointLightUniform must be less than 65536 bytes for vkCmdUpdateBuffer");
    static_assert(sizeof(PointLightUniform) % 4 == 0, "PointLightUniform size must be a multiple of 4 bytes");
    static_assert(offsetof(PointLightUniform, position) % 16 == 0, "cameraPosition must be aligned to 16 bytes");
    static_assert(offsetof(PointLightUniform, colour) % 16 == 0, "ambient must be aligned to 16 bytes");
}
