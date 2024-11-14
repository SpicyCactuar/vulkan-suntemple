#pragma once

#include "mesh.hpp"
#include "scene.hpp"
#include "../vkutils/vkimage.hpp"
#include "../vkutils/vkobject.hpp"
#include "../vkutils/vulkan_window.hpp"

namespace shadow {
    vkutils::RenderPass create_render_pass(const vkutils::VulkanWindow& window);

    vkutils::PipelineLayout create_opaque_pipeline_layout(const vkutils::VulkanContext& context,
                                                          const vkutils::DescriptorSetLayout& sceneLayout);

    vkutils::PipelineLayout create_alpha_pipeline_layout(const vkutils::VulkanContext& context,
                                                         const vkutils::DescriptorSetLayout& sceneLayout,
                                                         const vkutils::DescriptorSetLayout& materialLayout);

    vkutils::Pipeline create_opaque_pipeline(const vkutils::VulkanWindow& window,
                                             VkRenderPass renderPass,
                                             VkPipelineLayout pipelineLayout);

    vkutils::Pipeline create_alpha_pipeline(const vkutils::VulkanWindow& window,
                                            VkRenderPass renderPass,
                                            VkPipelineLayout pipelineLayout);

    std::tuple<vkutils::Image, vkutils::ImageView> create_shadow_framebuffer_image(
        const vkutils::VulkanWindow&, const vkutils::Allocator&);

    vkutils::Framebuffer create_shadow_framebuffer(const vkutils::VulkanWindow& window,
                                                   VkRenderPass shadowRenderPass,
                                                   VkImageView shadowView);

    void record_commands(VkCommandBuffer commandBuffer,
                         VkRenderPass renderPass,
                         VkFramebuffer framebuffer,
                         VkPipelineLayout opaquePipelineLayout,
                         VkPipeline opaqueShadowPipeline,
                         VkPipelineLayout alphaPipelineLayout,
                         VkPipeline alphaShadowPipeline,
                         VkBuffer sceneUBO,
                         const glsl::SceneUniform& sceneUniform,
                         VkDescriptorSet sceneDescriptors,
                         const std::vector<mesh::Mesh>& opaqueMeshes,
                         const std::vector<mesh::Mesh>& alphaMaskedMeshes,
                         const std::vector<VkDescriptorSet>& materialDescriptorSets);
}
