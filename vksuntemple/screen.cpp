#include "screen.hpp"

#include "../vkutils/to_string.hpp"
#include "../vkutils/error.hpp"

#include <array>

namespace screen {
    vkutils::DescriptorSetLayout create_descriptor_layout(const vkutils::VulkanContext& context) {
        constexpr std::array bindings{
            VkDescriptorSetLayoutBinding{
                .binding = 0, // layout(set = ..., binding = 0)
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            },
            VkDescriptorSetLayoutBinding{
                .binding = 1, // layout(set = ..., binding = 1)
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            }
        };

        const VkDescriptorSetLayoutCreateInfo layoutInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = bindings.size(),
            .pBindings = bindings.data()
        };

        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        if (const auto res = vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr, &layout);
            VK_SUCCESS != res) {
            throw vkutils::Error("Unable to create fullscreen descriptor set layout\n"
                                 "vkCreateDescriptorSetLayout() returned %s", vkutils::to_string(res).c_str()
            );
        }

        return vkutils::DescriptorSetLayout(context.device, layout);
    }

    vkutils::Buffer create_screen_effects_ubo(const vkutils::Allocator& allocator) {
        return vkutils::create_buffer(
            allocator,
            sizeof(glsl::ScreenEffectsUniform),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            0,
            VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
        );
    }

    void update_descriptor_set(const vkutils::VulkanContext& context,
                               const VkDescriptorSet screenDescriptorSet,
                               const vkutils::Sampler& offscreenSampler,
                               const VkImageView offscreenView,
                               const vkutils::Buffer& screenEffectsUBO) {
        const VkDescriptorImageInfo descriptorImageInfo{
            .sampler = offscreenSampler.handle,
            .imageView = offscreenView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        const VkDescriptorBufferInfo descriptorBufferInfo{
            .buffer = screenEffectsUBO.buffer,
            .range = VK_WHOLE_SIZE
        };

        const std::array writeDescriptor = {
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = screenDescriptorSet,
                .dstBinding = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &descriptorImageInfo,
            },
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = screenDescriptorSet,
                .dstBinding = 1,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &descriptorBufferInfo
            }
        };

        vkUpdateDescriptorSets(context.device, writeDescriptor.size(), writeDescriptor.data(), 0, nullptr);
    }

    glsl::ScreenEffectsUniform create_uniform(const state::State& state) {
        return glsl::ScreenEffectsUniform{
            .toneMappingEnabled = static_cast<std::uint32_t>(state.toneMappingEnabled)
        };
    }

    void update_screen_effects_ubo(const VkCommandBuffer commandBuffer,
                                   const VkBuffer screenEffectsUBO,
                                   const glsl::ScreenEffectsUniform& screenEffectsUniform) {
        vkutils::buffer_barrier(commandBuffer,
                                screenEffectsUBO,
                                VK_ACCESS_UNIFORM_READ_BIT,
                                VK_ACCESS_TRANSFER_WRITE_BIT,
                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                VK_PIPELINE_STAGE_TRANSFER_BIT
        );

        vkCmdUpdateBuffer(commandBuffer, screenEffectsUBO, 0, sizeof(glsl::ScreenEffectsUniform),
                          &screenEffectsUniform);

        vkutils::buffer_barrier(commandBuffer,
                                screenEffectsUBO,
                                VK_ACCESS_TRANSFER_WRITE_BIT,
                                VK_ACCESS_UNIFORM_READ_BIT,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }
}
