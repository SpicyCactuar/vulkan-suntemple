#pragma once

#include <array>

#include "../vkutils/vulkan_context.hpp"
#include "../vkutils/vulkan_window.hpp"

#include "screen.hpp"

namespace fullscreen {
    vkutils::RenderPass create_render_pass(const vkutils::VulkanWindow& window);

    vkutils::PipelineLayout create_pipeline_layout(const vkutils::VulkanContext& context,
                                                    const vkutils::DescriptorSetLayout& descriptorLayout);

    vkutils::Pipeline create_fullscreen_pipeline(const vkutils::VulkanWindow& window,
                                                  VkRenderPass renderPass,
                                                  VkPipelineLayout pipelineLayout);

    void prepare_frame_command_buffer(const vkutils::VulkanWindow& vulkanWindow,
                                      const vkutils::Fence& frameFence,
                                      VkCommandBuffer frameCommandBuffer);

    void record_commands(VkCommandBuffer commandBuffer,
                         VkRenderPass renderPass,
                         VkFramebuffer framebuffer,
                         VkPipelineLayout pipelineLayout,
                         VkPipeline fullscreenPipeline,
                         const VkExtent2D& imageExtent,
                         const glsl::ScreenEffectsUniform& screeEffectsUniform,
                         VkDescriptorSet fullscreenDescriptor,
                         VkBuffer screenEffectsUBO);

    void submit_frame_command_buffer(const vkutils::VulkanContext& context,
                                     VkCommandBuffer frameCommandBuffer,
                                     const std::array<VkSemaphore, 2>& waitSemaphores,
                                     const VkSemaphore& signalSemaphore,
                                     const vkutils::Fence& frameFence);
}
