#pragma once

#include "../vkutils/vkimage.hpp"
#include "../vkutils/vkobject.hpp"
#include "../vkutils/vulkan_window.hpp"

#include "mesh.hpp"
#include "scene.hpp"
#include "shade.hpp"

namespace offscreen {
    vkutils::RenderPass create_render_pass(const vkutils::VulkanWindow& window);

    vkutils::PipelineLayout create_pipeline_layout(const vkutils::VulkanContext& context,
                                                   const vkutils::DescriptorSetLayout& sceneLayout,
                                                   const vkutils::DescriptorSetLayout& shadeLayout,
                                                   const vkutils::DescriptorSetLayout& materialLayout);

    vkutils::Pipeline create_opaque_pipeline(const vkutils::VulkanWindow& window,
                                             VkRenderPass renderPass,
                                             VkPipelineLayout pipelineLayout);

    vkutils::Pipeline create_alpha_pipeline(const vkutils::VulkanWindow& window,
                                            VkRenderPass renderPass,
                                            VkPipelineLayout pipelineLayout);

    std::tuple<vkutils::Image, vkutils::ImageView> create_depth_buffer(
        const vkutils::VulkanWindow&, const vkutils::Allocator&);

    std::tuple<vkutils::Image, vkutils::ImageView> create_offscreen_target(const vkutils::VulkanWindow& window,
                                                                           const vkutils::Allocator& allocator);

    vkutils::Framebuffer create_offscreen_framebuffer(const vkutils::VulkanWindow& window,
                                                      VkRenderPass renderPass,
                                                      VkImageView offscreenView,
                                                      VkImageView depthView);

    void prepare_offscreen_command_buffer(const vkutils::VulkanContext& context,
                                          const vkutils::Fence& offscreenFence,
                                          VkCommandBuffer offscreenCommandBuffer);

    void record_commands(VkCommandBuffer commandBuffer,
                         VkRenderPass renderPass,
                         VkFramebuffer framebuffer,
                         VkPipelineLayout graphicsLayout,
                         VkPipeline opaquePipeline,
                         VkPipeline alphaMaskPipeline,
                         const VkExtent2D& imageExtent,
                         VkBuffer sceneUBO,
                         const glsl::SceneUniform& sceneUniform,
                         VkDescriptorSet sceneDescriptors,
                         VkBuffer shadeUBO,
                         const glsl::ShadeUniform& shadeUniform,
                         VkDescriptorSet screenDescriptors,
                         const std::vector<mesh::Mesh>& opaqueMeshes,
                         const std::vector<mesh::Mesh>& alphaMaskedMeshes,
                         const std::vector<VkDescriptorSet>& materialDescriptorSets);

    void submit_commands(const vkutils::VulkanContext& context,
                         VkCommandBuffer offscreenCommandBuffer,
                         const vkutils::Semaphore& signalSemaphore,
                         const vkutils::Fence& offscreenFence);

    void wait_offscreen_early(const vkutils::VulkanWindow& vulkanWindow,
                              const vkutils::Semaphore& waitSemaphore);
}
