#include "material.hpp"

#include <array>

#include "config.hpp"
#include "texture.hpp"
#include "../vkutils/error.hpp"
#include "../vkutils/to_string.hpp"

namespace material {
    bool Material::is_alpha_masked() const {
        return alphaMask.has_value();
    }

    void load_material_texture(const baked::BakedModel& model,
                               const std::uint32_t textureId,
                               const VkFormat format,
                               const vkutils::VulkanContext& context,
                               const vkutils::Allocator& allocator,
                               const vkutils::CommandPool& loadCommandPool,
                               std::vector<vkutils::Image>& textures) {
        if (textures[textureId].image != VK_NULL_HANDLE) {
            return;
        }

        const auto bakedTexture = model.textures[textureId];
        textures[textureId] = texture_to_image(
            context, texture::Texture(bakedTexture.path), format, allocator, loadCommandPool);
    }

    MaterialStore extract_materials(const baked::BakedModel& model,
                                    const vkutils::VulkanContext& context,
                                    const vkutils::Allocator& allocator) {
        std::vector<vkutils::Image> textures;
        textures.resize(model.textures.size());
        std::vector<Material> materials;
        materials.reserve(model.materials.size());
        const vkutils::CommandPool loadCommandPool = vkutils::create_command_pool(
            context, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

        for (const auto& modelMaterial : model.materials) {
            load_material_texture(model, modelMaterial.baseColorTextureId, Material::COLOUR_FORMAT,
                                  context, allocator, loadCommandPool, textures);
            load_material_texture(model, modelMaterial.roughnessTextureId, Material::LINEAR_FORMAT,
                                  context, allocator, loadCommandPool, textures);
            load_material_texture(model, modelMaterial.metalnessTextureId, Material::LINEAR_FORMAT,
                                  context, allocator, loadCommandPool, textures);
            load_material_texture(model, modelMaterial.normalMapTextureId, Material::LINEAR_FORMAT,
                                  context, allocator, loadCommandPool, textures);
            if (modelMaterial.alphaMaskTextureId != baked::NO_ID) {
                load_material_texture(model, modelMaterial.alphaMaskTextureId, Material::COLOUR_FORMAT,
                                      context, allocator, loadCommandPool, textures);
            }

            assert(textures[modelMaterial.baseColorTextureId].image != VK_NULL_HANDLE);
            assert(textures[modelMaterial.roughnessTextureId].image != VK_NULL_HANDLE);
            assert(textures[modelMaterial.metalnessTextureId].image != VK_NULL_HANDLE);
            assert(textures[modelMaterial.normalMapTextureId].image != VK_NULL_HANDLE);
            if (modelMaterial.alphaMaskTextureId != baked::NO_ID) {
                assert(textures[modelMaterial.alphaMaskTextureId].image != VK_NULL_HANDLE);
            }

            // Marked as [[maybe_unused]] to avoid generating warnings in release mode
            // Variable is only accessed by assert(...) calls, only relevant in debug mode
            [[maybe_unused]] const auto& material = materials.emplace_back(Material{
                .baseColour = vkutils::image_to_view(context, textures[modelMaterial.baseColorTextureId].image,
                                                     Material::COLOUR_FORMAT),
                .roughness = vkutils::image_to_view(context, textures[modelMaterial.roughnessTextureId].image,
                                                    Material::LINEAR_FORMAT),
                .metalness = vkutils::image_to_view(context, textures[modelMaterial.metalnessTextureId].image,
                                                    Material::LINEAR_FORMAT),
                .normalMap = vkutils::image_to_view(context, textures[modelMaterial.normalMapTextureId].image,
                                                    Material::LINEAR_FORMAT),
                .alphaMask = modelMaterial.alphaMaskTextureId != baked::NO_ID
                                 ? std::make_optional(
                                     vkutils::image_to_view(context, textures[modelMaterial.alphaMaskTextureId].image,
                                                            Material::COLOUR_FORMAT))
                                 : std::nullopt
            });

            assert(material.baseColour.handle != VK_NULL_HANDLE);
            assert(material.roughness.handle != VK_NULL_HANDLE);
            assert(material.metalness.handle != VK_NULL_HANDLE);
            assert(material.normalMap.handle != VK_NULL_HANDLE);
            assert(material.alphaMask.has_value() == (modelMaterial.alphaMaskTextureId != baked::NO_ID));
        }

        return MaterialStore{
            .textures = std::move(textures),
            .materials = std::move(materials)
        };
    }

    vkutils::DescriptorSetLayout create_descriptor_layout(const vkutils::VulkanContext& context) {
        constexpr std::array bindings = {
            // Base Colour
            VkDescriptorSetLayoutBinding{
                .binding = 0, // layout(set = ..., binding = 0)
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
            },
            // Roughness
            VkDescriptorSetLayoutBinding{
                .binding = 1, // layout(set = ..., binding = 1)
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
            },
            // Metalness
            VkDescriptorSetLayoutBinding{
                .binding = 2, // layout(set = ..., binding = 2)
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
            },
            // Normal Map
            VkDescriptorSetLayoutBinding{
                .binding = 3, // layout(set = ..., binding = 3)
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            },
            // Alpha Mask - Only for alpha_mask.frag
            VkDescriptorSetLayoutBinding{
                .binding = 4, // layout(set = ..., binding = 4)
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
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
            throw vkutils::Error("Unable to create descriptor set layout\n"
                                 "vkCreateDescriptorSetLayout() returned %s", vkutils::to_string(res).c_str()
            );
        }

        return vkutils::DescriptorSetLayout(context.device, layout);
    }

    template<size_t N>
    void update_material_descriptor_set(const vkutils::VulkanContext& context,
                                        const VkDescriptorSet materialDescriptorSet,
                                        const std::array<const VkDescriptorImageInfo, N>& textureDescriptors) {
        std::array<VkWriteDescriptorSet, N> writeDescriptor{};

        for (unsigned int i = 0; i < writeDescriptor.size(); ++i) {
            writeDescriptor[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptor[i].dstSet = materialDescriptorSet;
            writeDescriptor[i].dstBinding = i;
            writeDescriptor[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writeDescriptor[i].descriptorCount = 1;
            writeDescriptor[i].pImageInfo = &textureDescriptors[i];
        }

        vkUpdateDescriptorSets(context.device, writeDescriptor.size(), writeDescriptor.data(), 0, nullptr);
    }

    void update_descriptor_set(const vkutils::VulkanContext& context,
                               const VkDescriptorSet materialDescriptorSet,
                               const material::Material& material,
                               const vkutils::Sampler& anisotropySampler,
                               const vkutils::Sampler& pointSampler) {
        if (material.is_alpha_masked()) {
            const std::array<const VkDescriptorImageInfo, 5> textureDescriptors = {
                VkDescriptorImageInfo{
                    .sampler = anisotropySampler.handle,
                    .imageView = material.baseColour.handle,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                },
                VkDescriptorImageInfo{
                    .sampler = pointSampler.handle,
                    .imageView = material.roughness.handle,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                },
                VkDescriptorImageInfo{
                    .sampler = pointSampler.handle,
                    .imageView = material.metalness.handle,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                },
                VkDescriptorImageInfo{
                    .sampler = anisotropySampler.handle,
                    .imageView = material.normalMap.handle,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                },
                VkDescriptorImageInfo{
                    .sampler = pointSampler.handle,
                    .imageView = material.alphaMask.value().handle,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                }
            };
            update_material_descriptor_set(context, materialDescriptorSet, textureDescriptors);
        } else {
            const std::array<const VkDescriptorImageInfo, 4> textureDescriptors = {
                VkDescriptorImageInfo{
                    .sampler = anisotropySampler.handle,
                    .imageView = material.baseColour.handle,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                },
                VkDescriptorImageInfo{
                    .sampler = pointSampler.handle,
                    .imageView = material.roughness.handle,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                },
                VkDescriptorImageInfo{
                    .sampler = pointSampler.handle,
                    .imageView = material.metalness.handle,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                },
                VkDescriptorImageInfo{
                    .sampler = anisotropySampler.handle,
                    .imageView = material.normalMap.handle,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                }
            };
            update_material_descriptor_set(context, materialDescriptorSet, textureDescriptors);
        }
    }
}
