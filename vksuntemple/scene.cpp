#include "scene.hpp"

#include <array>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "config.hpp"
#include "../vkutils/error.hpp"
#include "../vkutils/to_string.hpp"
#include "../vkutils/vkutil.hpp"

#include "material.hpp"

namespace scene {
    vkutils::DescriptorSetLayout create_descriptor_layout(const vkutils::VulkanContext& context) {
        constexpr std::array bindings{
            VkDescriptorSetLayoutBinding{
                // number must match the index of the corresponding
                // binding = N declaration in the shader(s)!
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
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
            throw vkutils::Error("Unable to create descriptor set layout\n"
                                 "vkCreateDescriptorSetLayout() returned %s", vkutils::to_string(res).c_str()
            );
        }

        return vkutils::DescriptorSetLayout(context.device, layout);
    }

    vkutils::Buffer create_scene_ubo(const vkutils::Allocator& allocator) {
        return vkutils::create_buffer(
            allocator,
            sizeof(glsl::SceneUniform),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            0,
            VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
        );
    }

    void update_descriptor_set(const vkutils::VulkanContext& context,
                               const vkutils::Buffer& sceneUBO,
                               const VkDescriptorSet sceneDescriptorSet) {
        const VkDescriptorBufferInfo sceneUboInfo{
            .buffer = sceneUBO.buffer,
            .range = VK_WHOLE_SIZE
        };

        const std::array writeDescriptor{
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = sceneDescriptorSet,
                .dstBinding = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &sceneUboInfo
            }
        };

        vkUpdateDescriptorSets(context.device, writeDescriptor.size(), writeDescriptor.data(), 0, nullptr);
    }

    glsl::SceneUniform create_uniform(const std::uint32_t framebufferWidth,
                                      const std::uint32_t framebufferHeight,
                                      const state::State& state) {
        // Camera
        float const aspect = framebufferWidth / static_cast<float>(framebufferHeight);
        glm::mat4 cameraP = glm::perspectiveRH_ZO(
            vkutils::Radians(cfg::cameraFov).value(),
            aspect,
            cfg::cameraNear,
            cfg::cameraFar
        );
        cameraP[1][1] *= -1.0f; // mirror Y axis
        const glm::mat4 CV = glm::inverse(state.camera2world);
        const glm::mat4 VP = cameraP * CV;

        // Light
        glm::mat4 lightP = glm::perspectiveRH_ZO(
            vkutils::Radians(cfg::lightFov).value(),
            1.0f,
            cfg::lightNear,
            cfg::lightFar
        );
        lightP[1][1] *= -1.0f; // mirror Y axis
        // Look forward
        const glm::mat4 LV = glm::lookAtRH(cfg::lightPosition,
                                           cfg::lightLookCenter,
                                           {0.0f, 1.0f, 0.0f});
        const glm::mat4 LP = lightP * LV;

        return glsl::SceneUniform{
            .VP = VP,
            .LP = LP,
            .SLP = cfg::shadowTransformationMatrix * LP
        };
    }

    void update_scene_ubo(const VkCommandBuffer commandBuffer,
                          const VkBuffer sceneUBO,
                          const glsl::SceneUniform& sceneUniform) {
        vkutils::buffer_barrier(commandBuffer,
                                sceneUBO,
                                VK_ACCESS_UNIFORM_READ_BIT,
                                VK_ACCESS_TRANSFER_WRITE_BIT,
                                VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                                VK_PIPELINE_STAGE_TRANSFER_BIT
        );

        vkCmdUpdateBuffer(commandBuffer, sceneUBO, 0, sizeof(glsl::SceneUniform), &sceneUniform);

        vkutils::buffer_barrier(commandBuffer,
                                sceneUBO,
                                VK_ACCESS_TRANSFER_WRITE_BIT,
                                VK_ACCESS_UNIFORM_READ_BIT,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
        );
    }
}
