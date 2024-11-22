#include <chrono>
#include <cstdint>
#include <cstdio>
#include <tuple>
#include <vector>
#include <volk/volk.h>

#include "../vkutils/vkbuffer.hpp"
#include "../vkutils/vkimage.hpp"
#include "../vkutils/vkobject.hpp"
#include "../vkutils/vkutil.hpp"
#include "../vkutils/vulkan_window.hpp"

#include "baked_model.hpp"
#include "config.hpp"
#include "fullscreen.hpp"
#include "glfw.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "offscreen.hpp"
#include "scene.hpp"
#include "screen.hpp"
#include "shade.hpp"
#include "shadow.hpp"
#include "state.hpp"
#include "swapchain.hpp"

int main() try {
    // Prepare Vulkan window
    vkutils::VulkanWindow vulkanWindow = vkutils::make_vulkan_window();

    // Configure the GLFW callbacks & state
    state::State state{};

    glfw::setup_window(vulkanWindow, state);

    // Create VMA allocator
    const vkutils::Allocator allocator = vkutils::create_allocator(vulkanWindow);

    // Create descriptor layouts reused across shadow & offscreen passes
    const vkutils::DescriptorSetLayout sceneLayout = scene::create_descriptor_layout(vulkanWindow);
    const vkutils::DescriptorSetLayout materialLayout = material::create_descriptor_layout(vulkanWindow);

    // Initialise Shadow Pipeline
    const vkutils::RenderPass shadowPass = shadow::create_render_pass(vulkanWindow);
    const vkutils::PipelineLayout opaqueShadowLayout = shadow::create_opaque_pipeline_layout(vulkanWindow, sceneLayout);
    vkutils::Pipeline opaqueShadowPipeline = shadow::create_opaque_pipeline(
        vulkanWindow, shadowPass.handle, opaqueShadowLayout.handle);
    const vkutils::PipelineLayout alphaShadowLayout = shadow::create_alpha_pipeline_layout(
        vulkanWindow, sceneLayout, materialLayout);
    vkutils::Pipeline alphaShadowPipeline = shadow::create_alpha_pipeline(
        vulkanWindow, shadowPass.handle, alphaShadowLayout.handle);
    auto [shadowImage, shadowView] = shadow::create_shadow_framebuffer_image(vulkanWindow, allocator);
    const vkutils::Framebuffer shadowFramebuffer = shadow::create_shadow_framebuffer(
        vulkanWindow, shadowPass.handle, shadowView.handle);

    // Intialise Offscreen Pipeline
    const vkutils::RenderPass offscreenPass = offscreen::create_render_pass(vulkanWindow);
    const vkutils::DescriptorSetLayout shadeLayout = shade::create_descriptor_layout(vulkanWindow);
    const vkutils::PipelineLayout offscreenLayout = offscreen::create_pipeline_layout(
        vulkanWindow, sceneLayout, shadeLayout, materialLayout);
    vkutils::Pipeline offscreenOpaquePipeline = offscreen::create_opaque_pipeline(vulkanWindow, offscreenPass.handle,
        offscreenLayout.handle);
    vkutils::Pipeline offscreenAlphaPipeline = offscreen::create_alpha_pipeline(vulkanWindow, offscreenPass.handle,
        offscreenLayout.handle);
    auto [depthBuffer, depthBufferView] = offscreen::create_depth_buffer(vulkanWindow, allocator);
    auto [offscreenImage, offscreenView] = offscreen::create_offscreen_target(vulkanWindow, allocator);
    vkutils::Framebuffer offscreenFramebuffer = offscreen::create_offscreen_framebuffer(
        vulkanWindow, offscreenPass.handle, offscreenView.handle, depthBufferView.handle);

    // Intialise Fullscreen Pipeline
    vkutils::RenderPass fullscreenPass = fullscreen::create_render_pass(vulkanWindow);
    const vkutils::DescriptorSetLayout screenDescriptorLayout = screen::create_descriptor_layout(vulkanWindow);
    const vkutils::PipelineLayout fullscreenLayout = fullscreen::create_pipeline_layout(
        vulkanWindow, screenDescriptorLayout);
    vkutils::Pipeline fullscreenPipeline = fullscreen::create_fullscreen_pipeline(
        vulkanWindow, fullscreenPass.handle, fullscreenLayout.handle);

    // Initialise command pool
    const vkutils::CommandPool commandPool = vkutils::create_command_pool(
        vulkanWindow, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    // Initialise per-frame Framebuffers and Synchronisation resources
    std::vector<vkutils::Framebuffer> framebuffers = swapchain::create_swapchain_framebuffers(
        vulkanWindow, fullscreenPass.handle);

    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<vkutils::Fence> fences;

    for (std::size_t i = 0; i < framebuffers.size(); ++i) {
        commandBuffers.emplace_back(vkutils::alloc_command_buffer(vulkanWindow, commandPool.handle));
        fences.emplace_back(vkutils::create_fence(vulkanWindow, VK_FENCE_CREATE_SIGNALED_BIT));
    }

    // Initialise offscreen synchronisation resources
    const vkutils::Fence offscreenFence = vkutils::create_fence(vulkanWindow, VK_FENCE_CREATE_SIGNALED_BIT);
    const VkCommandBuffer offscreenCommandBuffer = vkutils::alloc_command_buffer(vulkanWindow, commandPool.handle);

    // Initialise semaphores
    vkutils::Semaphore offscreenFinished = vkutils::create_semaphore(vulkanWindow);
    const vkutils::Semaphore swapchainImageAvailable = vkutils::create_semaphore(vulkanWindow);
    const vkutils::Semaphore renderFinished = vkutils::create_semaphore(vulkanWindow);

    // Initialise descriptor pool
    const vkutils::DescriptorPool descriptorPool = vkutils::create_descriptor_pool(vulkanWindow);

    // Create Samplers
    const vkutils::Sampler anisotropySampler = vkutils::create_anisotropy_sampler(vulkanWindow);
    const vkutils::Sampler pointSampler = vkutils::create_point_sampler(vulkanWindow);
    const vkutils::Sampler screenSampler = vkutils::create_screen_sampler(vulkanWindow);
    const vkutils::Sampler shadowSampler = vkutils::create_shadow_sampler(vulkanWindow);

    // Load scene data
    vkutils::Buffer sceneUBO = scene::create_scene_ubo(allocator);
    const VkDescriptorSet sceneDescriptorSet = vkutils::allocate_descriptor_set(
        vulkanWindow, descriptorPool.handle, sceneLayout.handle);
    scene::update_descriptor_set(vulkanWindow, sceneUBO, sceneDescriptorSet);

    // Load shade descriptor
    vkutils::Buffer shadeUbo = shade::create_shade_ubo(allocator);
    const VkDescriptorSet shadeDescriptorSet = vkutils::allocate_descriptor_set(vulkanWindow, descriptorPool.handle,
        shadeLayout.handle);
    shade::update_descriptor_set(vulkanWindow, shadeUbo, shadeDescriptorSet, shadowSampler, shadowView.handle);

    // Load screen descriptor
    const VkDescriptorSet screenDescriptorSet = vkutils::allocate_descriptor_set(vulkanWindow, descriptorPool.handle,
        screenDescriptorLayout.handle);
    vkutils::Buffer screenEffectsUBO = screen::create_screen_effects_ubo(allocator);
    screen::update_descriptor_set(vulkanWindow, screenDescriptorSet, screenSampler, offscreenView.handle,
                                  screenEffectsUBO);

    // Load model
    const baked::BakedModel model = baked::load_baked_model(cfg::sunTempleObjZstdPath);

    // Load materials
    // Keeps both Images and ImageViews alive for the duration of the render loop
    const material::MaterialStore materialStore = material::extract_materials(model, vulkanWindow, allocator);

    // Load 1 DescriptorSet per material
    const std::vector<VkDescriptorSet> materialDescriptorSets = vkutils::allocate_descriptor_sets(
        vulkanWindow, descriptorPool.handle, materialLayout.handle,
        static_cast<std::uint32_t>(materialStore.materials.size()));

    for (std::size_t m = 0; m < materialStore.materials.size(); ++m) {
        const auto& material = materialStore.materials[m];
        const auto& materialDescriptorSet = materialDescriptorSets[m];

        material::update_descriptor_set(vulkanWindow, materialDescriptorSet, material, anisotropySampler, pointSampler);
    }

    // Categorise meshes into opaque & alpha masked
    const auto [opaqueMeshes, alphaMaskedMeshes] =
            mesh::extract_meshes(vulkanWindow, allocator, model, materialStore.materials);

    // Render loop
    bool recreateSwapchain = false;

    // Initialise clock right before the render loop
    auto lastClock = cfg::Clock::now();

    while (!glfwWindowShouldClose(vulkanWindow.window)) {
        // We want to render the next frame as soon as possible => Poll events
        glfwPollEvents();

        /** Acquire swapchain image, updates recreateSwapchain **/
        /** No resources were reserved for this loop **/

        // Recreate swapchain if needed
        if (recreateSwapchain) {
            // We need to destroy several objects, which may still be in use by the GPU.
            // Therefore, first wait for the GPU to finish processing.
            vkDeviceWaitIdle(vulkanWindow.device);

            // Recreate them
            const auto changes = recreate_swapchain(vulkanWindow);

            if (changes.changedFormat) {
                // Offscreen does not depend on swapchain format, only recreate Fullscreen pass
                fullscreenPass = fullscreen::create_render_pass(vulkanWindow);
                fullscreenPipeline = fullscreen::create_fullscreen_pipeline(
                    vulkanWindow, fullscreenPass.handle, fullscreenLayout.handle);
            }

            if (changes.changedSize) {
                std::tie(depthBuffer, depthBufferView) = offscreen::create_depth_buffer(vulkanWindow, allocator);
                std::tie(offscreenImage, offscreenView) = offscreen::create_offscreen_target(vulkanWindow, allocator);

                offscreenOpaquePipeline = offscreen::create_opaque_pipeline(vulkanWindow, offscreenPass.handle,
                                                                            offscreenLayout.handle);
                offscreenAlphaPipeline = offscreen::create_alpha_pipeline(vulkanWindow, offscreenPass.handle,
                                                                          offscreenLayout.handle);
                fullscreenPipeline = fullscreen::create_fullscreen_pipeline(
                    vulkanWindow, fullscreenPass.handle, fullscreenLayout.handle);

                screen::update_descriptor_set(vulkanWindow, screenDescriptorSet, screenSampler,
                                              offscreenView.handle, screenEffectsUBO);
                shade::update_descriptor_set(vulkanWindow, shadeUbo, shadeDescriptorSet, shadowSampler,
                                             shadowView.handle);
            }

            offscreenFramebuffer = offscreen::create_offscreen_framebuffer(
                vulkanWindow, offscreenPass.handle, offscreenView.handle, depthBufferView.handle);
            framebuffers = swapchain::create_swapchain_framebuffers(vulkanWindow, fullscreenPass.handle);

            recreateSwapchain = false;
            // Swapchain image has not been acquired yet, proceed with the loop
        }

        // Update state
        const auto now = cfg::Clock::now();
        const auto dt = std::chrono::duration_cast<cfg::Secondsf>(now - lastClock).count();
        lastClock = now;

        state::update_state(state, dt);

        // Update uniforms
        const glsl::SceneUniform sceneUniform = scene::create_uniform(
            vulkanWindow.swapchainExtent.width, vulkanWindow.swapchainExtent.height, state);
        const glsl::ShadeUniform shadeUniform = shade::create_uniform(state);
        const glsl::ScreenEffectsUniform screenEffectsUniform = screen::create_uniform(state);

        // Prepare Offscreen command buffer
        offscreen::prepare_offscreen_command_buffer(vulkanWindow, offscreenFence, offscreenCommandBuffer);

        // Record Shadow commands
        shadow::record_commands(
            offscreenCommandBuffer,
            shadowPass.handle,
            shadowFramebuffer.handle,
            opaqueShadowLayout.handle,
            opaqueShadowPipeline.handle,
            alphaShadowLayout.handle,
            alphaShadowPipeline.handle,
            sceneUBO.buffer,
            sceneUniform,
            sceneDescriptorSet,
            opaqueMeshes,
            alphaMaskedMeshes,
            materialDescriptorSets
        );

        // No need for explicity synchronisation here as Subpass dependencies guarantee it implicitly
        // See https://github.com/SaschaWillems/Vulkan/blob/master/examples/shadowmapping/shadowmapping.cpp#L312C1-L312C39

        // Record Offscreen commands
        offscreen::record_commands(
            offscreenCommandBuffer,
            offscreenPass.handle,
            offscreenFramebuffer.handle,
            offscreenLayout.handle,
            offscreenOpaquePipeline.handle,
            offscreenAlphaPipeline.handle,
            vulkanWindow.swapchainExtent,
            sceneUBO.buffer,
            sceneUniform,
            sceneDescriptorSet,
            shadeUbo.buffer,
            shadeUniform,
            shadeDescriptorSet,
            opaqueMeshes,
            alphaMaskedMeshes,
            materialDescriptorSets
        );

        // Submit Offscreen commands
        offscreen::submit_commands(vulkanWindow, offscreenCommandBuffer, offscreenFinished, offscreenFence);

        // Acquire next swap chain image, without waiting for offscreen commands to finish
        const std::uint32_t imageIndex = swapchain::acquire_swapchain_image(vulkanWindow, swapchainImageAvailable,
                                                                            recreateSwapchain);

        if (recreateSwapchain) {
            // Offscreen pass was submitted but offscreenFinished is not waited for
            // Need to wait on all semaphores that were started
            // Otherwise there is a validation error in the next loop iteration
            offscreen::wait_offscreen_early(vulkanWindow, offscreenFinished);
            continue;
        }

        // Retrieve per-frame pipeline resources
        assert(static_cast<std::size_t>(imageIndex) < fences.size());
        const vkutils::Fence& frameFence = fences[imageIndex];

        assert(static_cast<std::size_t>(imageIndex) < commandBuffers.size());
        assert(static_cast<std::size_t>(imageIndex) < framebuffers.size());

        const VkCommandBuffer frameCommandBuffer = commandBuffers[imageIndex];
        const vkutils::Framebuffer& fullscreenFramebuffer = framebuffers[imageIndex];

        // Begin Fullscreen command buffer
        fullscreen::prepare_frame_command_buffer(vulkanWindow, frameFence, frameCommandBuffer);

        // Record Fullscreen commands
        fullscreen::record_commands(
            frameCommandBuffer,
            fullscreenPass.handle,
            fullscreenFramebuffer.handle,
            fullscreenLayout.handle,
            fullscreenPipeline.handle,
            vulkanWindow.swapchainExtent,
            screenEffectsUniform,
            screenDescriptorSet,
            screenEffectsUBO.buffer
        );

        // Submit fullscreen commands, waits for both offscreenFinished and swapchainImageAvailable
        const std::array waitSemaphores = {
            offscreenFinished.handle, swapchainImageAvailable.handle
        };
        fullscreen::submit_frame_command_buffer(vulkanWindow, frameCommandBuffer,
                                                waitSemaphores, renderFinished.handle, frameFence);

        // Present the results after renderFinished is signalled
        swapchain::present_results(vulkanWindow.presentQueue, vulkanWindow.swapchain, imageIndex,
                                   renderFinished.handle, recreateSwapchain);
    }

    // Cleanup takes place automatically in the destructors, but we sill need
    // to ensure that all Vulkan commands have finished before that.
    vkDeviceWaitIdle(vulkanWindow.device);
    return EXIT_SUCCESS;
} catch (std::exception const& exception) {
    std::fprintf(stderr, "\n");
    std::fprintf(stderr, "Error: %s\n", exception.what());
    return EXIT_FAILURE;
}
