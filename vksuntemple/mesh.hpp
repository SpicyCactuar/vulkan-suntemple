#pragma once

#include <cstdint>
#include <vector>
#include <tuple>

#include "../vkutils/allocator.hpp"
#include "../vkutils/vkbuffer.hpp"
#include "../vkutils/vulkan_context.hpp"

#include "baked_model.hpp"
#include "material.hpp"

namespace glsl {
    struct MeshPushConstants {
        glm::vec3 colour;
    };
}

namespace mesh {
    struct Mesh {
        vkutils::Buffer positions;
        vkutils::Buffer uvs;
        vkutils::Buffer normals;
        vkutils::Buffer tangents;
        vkutils::Buffer indices;
        glsl::MeshPushConstants pushConstants;
        std::uint32_t materialId;

        std::uint32_t indexCount;
    };

    std::tuple<std::vector<Mesh>, std::vector<Mesh>> extract_meshes(const vkutils::VulkanContext&,
                                                                    const vkutils::Allocator&,
                                                                    const baked::BakedModel& model,
                                                                    const std::vector<material::Material>& materials);
}
