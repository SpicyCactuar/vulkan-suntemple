#pragma once

#include "../vkutils/allocator.hpp"
#include "../vkutils/vkbuffer.hpp"
#include "../vkutils/vkutil.hpp"
#include "../vkutils/vulkan_context.hpp"

#include "state.hpp"

namespace glsl {
    struct ScreenEffectsUniform {
        std::uint32_t toneMappingEnabled;
    };

    // We want to use vkCmdUpdateBuffer() to update the contents of our uniform buffers. vkCmdUpdateBuffer()
    // has a number of requirements, including the two below. See
    // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/vkCmdUpdateBuffer.html
    static_assert(sizeof(ScreenEffectsUniform) <= 65536,
                  "FullscreenUniform must be less than 65536 bytes for vkCmdUpdateBuffer");
    static_assert(sizeof(ScreenEffectsUniform) % 4 == 0, "FullscreenUniform size must be a multiple of 4 bytes");
}

namespace screen {
    vkutils::DescriptorSetLayout create_descriptor_layout(const vkutils::VulkanContext& context);

    vkutils::Buffer create_screen_effects_ubo(const vkutils::Allocator& allocator);

    void update_descriptor_set(const vkutils::VulkanContext& context,
                               VkDescriptorSet screenDescriptorSet,
                               const vkutils::Sampler& offscreenSampler,
                               VkImageView offscreenView,
                               const vkutils::Buffer& screenEffectsUBO);

    glsl::ScreenEffectsUniform create_uniform(const state::State& state);

    void update_screen_effects_ubo(VkCommandBuffer commandBuffer,
                                   VkBuffer screenEffectsUBO,
                                   const glsl::ScreenEffectsUniform& screenEffectsUniform);
}
