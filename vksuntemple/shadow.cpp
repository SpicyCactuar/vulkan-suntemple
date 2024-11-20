#include "shadow.hpp"

#include <array>
#include <tuple>

#include "../vkutils/to_string.hpp"
#include "../vkutils/error.hpp"
#include "../vkutils/vkutil.hpp"

#include "config.hpp"

namespace shadow {
    vkutils::RenderPass create_render_pass(const vkutils::VulkanWindow& window) {
        constexpr std::array attachments{
            VkAttachmentDescription{
                .format = cfg::depthFormat,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
            }
        };

        constexpr VkAttachmentReference depthAttachment{
            .attachment = 0, // attachments[0]
            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        };

        const std::array subpasses{
            VkSubpassDescription{
                .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .colorAttachmentCount = 0,
                .pColorAttachments = nullptr,
                .pDepthStencilAttachment = &depthAttachment
            }
        };

        // Requires a subpass dependency to ensure that the first transition happens after the presentation engine is
        // done with it.
        // https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples-(Legacy-synchronization-APIs)#swapchain-image-acquire-and-present
        constexpr std::array subpassDependencies{
            VkSubpassDependency{
                .srcSubpass = VK_SUBPASS_EXTERNAL,
                .dstSubpass = 0,
                .srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                .srcAccessMask = 0,
                .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
            },
            VkSubpassDependency{
                .srcSubpass = VK_SUBPASS_EXTERNAL,
                .dstSubpass = 0,
                .srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
            }
        };

        // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkRenderPassCreateInfo.html
        const VkRenderPassCreateInfo passInfo{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = attachments.size(),
            .pAttachments = attachments.data(),
            .subpassCount = subpasses.size(),
            .pSubpasses = subpasses.data(),
            .dependencyCount = subpassDependencies.size(),
            .pDependencies = subpassDependencies.data()
        };

        VkRenderPass renderPass = VK_NULL_HANDLE;
        if (const auto res = vkCreateRenderPass(window.device, &passInfo, nullptr, &renderPass);
            VK_SUCCESS != res) {
            throw vkutils::Error("Unable to create shadow map render pass\n"
                                 "vkCreateRenderPass() returned %s", vkutils::to_string(res).c_str()
            );
        }

        return vkutils::RenderPass(window.device, renderPass);
    }

    vkutils::PipelineLayout create_opaque_pipeline_layout(const vkutils::VulkanContext& context,
                                                          const vkutils::DescriptorSetLayout& sceneLayout) {
        const std::array layouts = {
            // Order must match the set = N in the shaders
            sceneLayout.handle, // set 0
        };

        const VkPipelineLayoutCreateInfo layoutInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            // Initialise with layouts information
            .setLayoutCount = layouts.size(),
            .pSetLayouts = layouts.data(),
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr
        };

        VkPipelineLayout layout = VK_NULL_HANDLE;
        if (const auto res = vkCreatePipelineLayout(context.device, &layoutInfo, nullptr, &layout);
            VK_SUCCESS != res) {
            throw vkutils::Error("Unable to create opaque shadow pipeline layout\n"
                                 "vkCreatePipelineLayout() returned %s", vkutils::to_string(res).c_str());
        }

        return vkutils::PipelineLayout(context.device, layout);
    }

    vkutils::PipelineLayout create_alpha_pipeline_layout(const vkutils::VulkanContext& context,
                                                         const vkutils::DescriptorSetLayout& sceneLayout,
                                                         const vkutils::DescriptorSetLayout& materialLayout) {
        const std::array layouts = {
            // Order must match the set = N in the shaders
            sceneLayout.handle, // set 0
            materialLayout.handle // set 1
        };

        const VkPipelineLayoutCreateInfo layoutInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            // Initialise with layouts information
            .setLayoutCount = layouts.size(),
            .pSetLayouts = layouts.data(),
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr
        };

        VkPipelineLayout layout = VK_NULL_HANDLE;
        if (const auto res = vkCreatePipelineLayout(context.device, &layoutInfo, nullptr, &layout);
            VK_SUCCESS != res) {
            throw vkutils::Error("Unable to create alpha shadow pipeline layout\n"
                                 "vkCreatePipelineLayout() returned %s", vkutils::to_string(res).c_str());
        }

        return vkutils::PipelineLayout(context.device, layout);
    }

    vkutils::Pipeline create_opaque_pipeline(const vkutils::VulkanWindow& window,
                                             const VkRenderPass renderPass,
                                             const VkPipelineLayout pipelineLayout) {
        // Load only vertex and fragment shader modules
        const vkutils::ShaderModule vert = vkutils::load_shader_module(window, cfg::opaqueShadowVertPath);
        const vkutils::ShaderModule frag = vkutils::load_shader_module(window, cfg::opaqueShadowFragPath);

        // Define shader stages in the pipeline
        const std::array stages = {
            // Vertex shader
            VkPipelineShaderStageCreateInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = vert.handle,
                .pName = "main"
            },
            // Fragment shader
            VkPipelineShaderStageCreateInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = frag.handle,
                .pName = "main"
            }
        };

        // Create vertex inputs
        constexpr std::array vertexBindings = {
            // Positions Binding
            VkVertexInputBindingDescription{
                .binding = 0,
                .stride = sizeof(glm::vec3),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
            }
        };

        // Create vertex attributes
        constexpr std::array vertexAttributes = {
            // Positions attribute
            VkVertexInputAttributeDescription{
                .location = 0, // must match shader
                .binding = vertexBindings[0].binding,
                .format = VK_FORMAT_R32G32B32_SFLOAT, // (x, y, z)
                .offset = 0
            }
        };

        // Create Pipeline with Vertex input
        const VkPipelineVertexInputStateCreateInfo inputInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = vertexBindings.size(),
            .pVertexBindingDescriptions = vertexBindings.data(),
            .vertexAttributeDescriptionCount = vertexAttributes.size(),
            .pVertexAttributeDescriptions = vertexAttributes.data()
        };

        // Define which primitive (point, line, triangle, ...) the input is assembled into for rasterization.
        constexpr VkPipelineInputAssemblyStateCreateInfo assemblyInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE
        };

        constexpr VkRect2D scissor{
            .offset = VkOffset2D{0, 0},
            .extent = cfg::shadowMapExtent
        };

        // Define viewport and scissor regions
        constexpr VkViewport viewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(scissor.extent.width),
            .height = static_cast<float>(scissor.extent.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f
        };

        const VkPipelineViewportStateCreateInfo viewportInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports = &viewport,
            .scissorCount = 1,
            .pScissors = &scissor
        };

        // Define rasterisation options
        constexpr VkPipelineRasterizationStateCreateInfo rasterInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .depthBiasEnable = VK_TRUE,
            .depthBiasConstantFactor = 8.0f,
            .depthBiasClamp = 0.0f,
            .depthBiasSlopeFactor = 2.0f,
            .lineWidth = 1.0f // required.
        };

        // Define multisampling state
        constexpr VkPipelineMultisampleStateCreateInfo samplingInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
        };

        // Define depth info
        constexpr VkPipelineDepthStencilStateCreateInfo depthInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = VK_TRUE,
            .depthWriteEnable = VK_TRUE,
            .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
            .minDepthBounds = 0.0f,
            .maxDepthBounds = 1.0f
        };

        // Create pipeline
        const VkGraphicsPipelineCreateInfo pipelineInfo{
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = stages.size(),
            .pStages = stages.data(),
            .pVertexInputState = &inputInfo,
            .pInputAssemblyState = &assemblyInfo,
            .pTessellationState = nullptr, // no tessellation
            .pViewportState = &viewportInfo,
            .pRasterizationState = &rasterInfo,
            .pMultisampleState = &samplingInfo,
            .pDepthStencilState = &depthInfo,
            .pColorBlendState = nullptr, // no colour
            .pDynamicState = nullptr, // no dynamic states
            .layout = pipelineLayout,
            .renderPass = renderPass,
            .subpass = 0 // first subpass of renderPass
        };

        VkPipeline pipeline = VK_NULL_HANDLE;
        if (const auto res = vkCreateGraphicsPipelines(window.device, VK_NULL_HANDLE,
                                                       1, &pipelineInfo, nullptr, &pipeline);
            VK_SUCCESS != res) {
            throw vkutils::Error("Unable to create shadow map pipeline\n"
                                 "vkCreateGraphicsPipelines() returned %s", vkutils::to_string(res).c_str());
        }

        return vkutils::Pipeline(window.device, pipeline);
    }

    vkutils::Pipeline create_alpha_pipeline(const vkutils::VulkanWindow& window,
                                            const VkRenderPass renderPass,
                                            const VkPipelineLayout pipelineLayout) {
        // Load only vertex and fragment shader modules
        const vkutils::ShaderModule vert = vkutils::load_shader_module(window, cfg::alphaShadowVertPath);
        const vkutils::ShaderModule frag = vkutils::load_shader_module(window, cfg::alphaShadowFragPath);

        // Define shader stages in the pipeline
        const std::array stages = {
            // Vertex shader
            VkPipelineShaderStageCreateInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = vert.handle,
                .pName = "main"
            },
            // Fragment shader
            VkPipelineShaderStageCreateInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = frag.handle,
                .pName = "main"
            }
        };

        // Create vertex inputs
        constexpr std::array vertexBindings = {
            // Positions Binding
            VkVertexInputBindingDescription{
                .binding = 0,
                .stride = sizeof(glm::vec3),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
            },
            // UVs Binding
            VkVertexInputBindingDescription{
                .binding = 1,
                .stride = sizeof(glm::vec2),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
            }
        };

        // Create vertex attributes
        constexpr std::array vertexAttributes = {
            // Positions attribute
            VkVertexInputAttributeDescription{
                .location = 0, // must match shader
                .binding = vertexBindings[0].binding,
                .format = VK_FORMAT_R32G32B32_SFLOAT, // (x, y, z)
                .offset = 0
            },
            // UVs attribute
            VkVertexInputAttributeDescription{
                .location = 1, // must match shader
                .binding = vertexBindings[1].binding,
                .format = VK_FORMAT_R32G32_SFLOAT, // (u, v)
                .offset = 0
            }
        };

        // Create Pipeline with Vertex input
        const VkPipelineVertexInputStateCreateInfo inputInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = vertexBindings.size(),
            .pVertexBindingDescriptions = vertexBindings.data(),
            .vertexAttributeDescriptionCount = vertexAttributes.size(),
            .pVertexAttributeDescriptions = vertexAttributes.data()
        };

        // Define which primitive (point, line, triangle, ...) the input is assembled into for rasterization.
        constexpr VkPipelineInputAssemblyStateCreateInfo assemblyInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE
        };

        constexpr VkRect2D scissor{
            .offset = VkOffset2D{0, 0},
            .extent = cfg::shadowMapExtent
        };

        // Define viewport and scissor regions
        constexpr VkViewport viewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(scissor.extent.width),
            .height = static_cast<float>(scissor.extent.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f
        };

        const VkPipelineViewportStateCreateInfo viewportInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports = &viewport,
            .scissorCount = 1,
            .pScissors = &scissor
        };

        // Define rasterisation options
        constexpr VkPipelineRasterizationStateCreateInfo rasterInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .depthBiasEnable = VK_TRUE,
            .depthBiasConstantFactor = 8.0f,
            .depthBiasClamp = 0.0f,
            .depthBiasSlopeFactor = 2.0f,
            .lineWidth = 1.0f // required.
        };

        // Define multisampling state
        constexpr VkPipelineMultisampleStateCreateInfo samplingInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
        };

        // Define depth info
        constexpr VkPipelineDepthStencilStateCreateInfo depthInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = VK_TRUE,
            .depthWriteEnable = VK_TRUE,
            .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
            .minDepthBounds = 0.0f,
            .maxDepthBounds = 1.0f
        };

        // Create pipeline
        const VkGraphicsPipelineCreateInfo pipelineInfo{
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = stages.size(),
            .pStages = stages.data(),
            .pVertexInputState = &inputInfo,
            .pInputAssemblyState = &assemblyInfo,
            .pTessellationState = nullptr, // no tessellation
            .pViewportState = &viewportInfo,
            .pRasterizationState = &rasterInfo,
            .pMultisampleState = &samplingInfo,
            .pDepthStencilState = &depthInfo,
            .pColorBlendState = nullptr, // no colour
            .pDynamicState = nullptr, // no dynamic states
            .layout = pipelineLayout,
            .renderPass = renderPass,
            .subpass = 0 // first subpass of renderPass
        };

        VkPipeline pipeline = VK_NULL_HANDLE;
        if (const auto res = vkCreateGraphicsPipelines(window.device, VK_NULL_HANDLE,
                                                       1, &pipelineInfo, nullptr, &pipeline);
            VK_SUCCESS != res) {
            throw vkutils::Error("Unable to create shadow map pipeline\n"
                                 "vkCreateGraphicsPipelines() returned %s", vkutils::to_string(res).c_str());
        }

        return vkutils::Pipeline(window.device, pipeline);
    }

    std::tuple<vkutils::Image, vkutils::ImageView> create_shadow_framebuffer_image(const vkutils::VulkanWindow& window,
        const vkutils::Allocator& allocator) {
        constexpr VkImageCreateInfo imageInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = cfg::depthFormat,
            .extent = VkExtent3D{
                .width = cfg::shadowMapExtent.width,
                .height = cfg::shadowMapExtent.height,
                .depth = 1,
            },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
        };

        constexpr VmaAllocationCreateInfo allocInfo{
            .usage = VMA_MEMORY_USAGE_GPU_ONLY
        };

        VkImage image = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;

        if (const auto res = vmaCreateImage(allocator.allocator, &imageInfo, &allocInfo, &image, &allocation, nullptr);
            VK_SUCCESS != res) {
            throw vkutils::Error("Unable to allocate shadow map image\n"
                                 "vmaCreateImage() returned %s", vkutils::to_string(res).c_str()
            );
        }

        vkutils::Image shadowImage(allocator.allocator, image, allocation);

        const VkImageViewCreateInfo viewInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = shadowImage.image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = cfg::depthFormat,
            .components = VkComponentMapping{},
            .subresourceRange = VkImageSubresourceRange{
                VK_IMAGE_ASPECT_DEPTH_BIT,
                0, 1,
                0, 1
            }
        };

        VkImageView view = VK_NULL_HANDLE;
        if (const auto res = vkCreateImageView(window.device, &viewInfo, nullptr, &view);
            VK_SUCCESS != res) {
            throw vkutils::Error("Unable to create depth buffer image view\n"
                                 "vkCreateImageView() returned %s", vkutils::to_string(res).c_str()
            );
        }

        return {std::move(shadowImage), vkutils::ImageView(window.device, view)};
    }

    vkutils::Framebuffer create_shadow_framebuffer(const vkutils::VulkanWindow& window,
                                                   const VkRenderPass shadowRenderPass,
                                                   const VkImageView shadowView) {
        const std::array attachments = {shadowView};
        const VkFramebufferCreateInfo framebufferInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = shadowRenderPass,
            .attachmentCount = attachments.size(),
            .pAttachments = attachments.data(),
            .width = cfg::shadowMapExtent.width,
            .height = cfg::shadowMapExtent.height,
            .layers = 1
        };

        VkFramebuffer framebuffer;
        if (const auto res = vkCreateFramebuffer(window.device, &framebufferInfo, nullptr, &framebuffer);
            VK_SUCCESS != res) {
            throw vkutils::Error("Unable to create offscreen framebuffer\n"
                                 "vkCreateImageView() returned %s", vkutils::to_string(res).c_str()
            );
        }

        return vkutils::Framebuffer(window.device, framebuffer);
    }

    void record_commands(const VkCommandBuffer commandBuffer,
                         const VkRenderPass renderPass,
                         const VkFramebuffer framebuffer,
                         const VkPipelineLayout opaquePipelineLayout,
                         const VkPipeline opaqueShadowPipeline,
                         const VkPipelineLayout alphaPipelineLayout,
                         const VkPipeline alphaShadowPipeline,
                         const VkBuffer sceneUBO,
                         const glsl::SceneUniform& sceneUniform,
                         const VkDescriptorSet sceneDescriptorSet,
                         const std::vector<mesh::Mesh>& opaqueMeshes,
                         const std::vector<mesh::Mesh>& alphaMaskedMeshes,
                         const std::vector<VkDescriptorSet>& materialDescriptorSets) {
        // Begin render pass
        constexpr std::array clearValues{
            // Clear depth value
            VkClearValue{
                .depthStencil = VkClearDepthStencilValue{
                    .depth = 1.0f
                }
            }
        };

        // Prepare uniforms
        scene::update_scene_ubo(commandBuffer, sceneUBO, sceneUniform);

        // Bind scene descriptor set into layout(set = 0, ...)
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                opaquePipelineLayout, 0, 1,
                                &sceneDescriptorSet, 0, nullptr);

        // Create render pass command
        const VkRenderPassBeginInfo passInfo{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = renderPass,
            .framebuffer = framebuffer,
            .renderArea = VkRect2D{
                .offset = VkOffset2D{0, 0},
                .extent = cfg::shadowMapExtent
            },
            .clearValueCount = clearValues.size(),
            .pClearValues = clearValues.data()
        };

        // Begin render pass
        vkCmdBeginRenderPass(commandBuffer, &passInfo, VK_SUBPASS_CONTENTS_INLINE);

        // First draw opaque pipeline
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, opaqueShadowPipeline);

        // Draw opaque meshes
        for (const auto& mesh : opaqueMeshes) {
            // Bind mesh vertex buffers into layout(location = {1})
            const std::array vertexBuffers = {mesh.positions.buffer};
            constexpr std::array<VkDeviceSize, vertexBuffers.size()> offsets{};
            vkCmdBindVertexBuffers(commandBuffer, 0, vertexBuffers.size(), vertexBuffers.data(), offsets.data());

            // Bind mesh vertex indices
            vkCmdBindIndexBuffer(commandBuffer, mesh.indices.buffer, 0, VK_INDEX_TYPE_UINT32);

            // Draw mesh vertices
            vkCmdDrawIndexed(commandBuffer, mesh.indexCount, 1, 0, 0, 0);
        }

        // Then draw alpha pipeline
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, alphaShadowPipeline);

        // Draw meshes
        for (const auto& mesh : alphaMaskedMeshes) {
            // Bind mesh descriptor set into layout(set = 1, ...)
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    alphaPipelineLayout, 1, 1,
                                    &materialDescriptorSets[mesh.materialId], 0, nullptr);

            // Bind mesh vertex buffers into layout(location = {1, 2})
            const std::array vertexBuffers = {mesh.positions.buffer, mesh.uvs.buffer};
            constexpr std::array<VkDeviceSize, vertexBuffers.size()> offsets{};
            vkCmdBindVertexBuffers(commandBuffer, 0, vertexBuffers.size(), vertexBuffers.data(), offsets.data());

            // Bind mesh vertex indices
            vkCmdBindIndexBuffer(commandBuffer, mesh.indices.buffer, 0, VK_INDEX_TYPE_UINT32);

            // Draw mesh vertices
            vkCmdDrawIndexed(commandBuffer, mesh.indexCount, 1, 0, 0, 0);
        }

        // End the render pass
        vkCmdEndRenderPass(commandBuffer);
    }
}
