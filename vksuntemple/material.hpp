#pragma once

#include <optional>

#include "../vkutils/vkimage.hpp"
#include "../vkutils/vkutil.hpp"
#include "../vkutils/vulkan_context.hpp"

#include "baked_model.hpp"
#include "material.hpp"

namespace material {
    struct Material {
        vkutils::ImageView baseColour;
        vkutils::ImageView roughness;
        vkutils::ImageView metalness;
        vkutils::ImageView normalMap;
        std::optional<vkutils::ImageView> alphaMask;

        bool is_alpha_masked() const;

        static constexpr VkFormat COLOUR_FORMAT = VK_FORMAT_R8G8B8A8_SRGB;
        static constexpr VkFormat LINEAR_FORMAT = VK_FORMAT_R8G8B8A8_UNORM;
    };

    struct MaterialStore {
        std::vector<vkutils::Image> textures;
        std::vector<Material> materials;
    };

    MaterialStore extract_materials(const baked::BakedModel& model,
                                    const vkutils::VulkanContext& context,
                                    const vkutils::Allocator& allocator);

    vkutils::DescriptorSetLayout create_descriptor_layout(const vkutils::VulkanContext&);

    void update_descriptor_set(const vkutils::VulkanContext& context,
                               VkDescriptorSet materialDescriptorSet,
                               const material::Material& material,
                               const vkutils::Sampler& anisotropySampler,
                               const vkutils::Sampler& pointSampler);
}
