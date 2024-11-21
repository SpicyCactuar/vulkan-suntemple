#include "glfw.hpp"

#include <iostream>
#include <GLFW/glfw3.h>

#include "config.hpp"
#include "state.hpp"

namespace {
    bool altKeyPressed = false;
}

namespace glfw {
    void input_state_callback(state::State* state, const int keyCode, const int action) {
        const bool isReleased = GLFW_RELEASE == action;

        switch (keyCode) {
            case GLFW_KEY_W:
                state->inputMap[static_cast<std::size_t>(state::InputState::forward)] = !isReleased;
                break;
            case GLFW_KEY_S:
                state->inputMap[static_cast<std::size_t>(state::InputState::backward)] = !isReleased;
                break;
            case GLFW_KEY_A:
                state->inputMap[static_cast<std::size_t>(state::InputState::strafeLeft)] = !isReleased;
                break;
            case GLFW_KEY_D:
                state->inputMap[static_cast<std::size_t>(state::InputState::strafeRight)] = !isReleased;
                break;
            case GLFW_KEY_E:
                state->inputMap[static_cast<std::size_t>(state::InputState::levitate)] = !isReleased;
                break;
            case GLFW_KEY_Q:
                state->inputMap[static_cast<std::size_t>(state::InputState::sink)] = !isReleased;
                break;
            case GLFW_KEY_LEFT_SHIFT:
                [[fallthrough]];
            case GLFW_KEY_RIGHT_SHIFT:
                state->inputMap[static_cast<std::size_t>(state::InputState::fast)] = !isReleased;
                break;
            case GLFW_KEY_LEFT_CONTROL:
                [[fallthrough]];
            case GLFW_KEY_RIGHT_CONTROL:
                state->inputMap[static_cast<std::size_t>(state::InputState::slow)] = !isReleased;
                break;
            default:
                break;
        }
    }

    void render_mode_callback(state::State* state, const int keyCode, const int action) {
        // Register alt key press/release
        if (GLFW_KEY_LEFT_ALT == keyCode || GLFW_KEY_RIGHT_ALT == keyCode) {
            switch (action) {
                case GLFW_PRESS:
                    altKeyPressed = true;
                    break;
                case GLFW_RELEASE:
                    altKeyPressed = false;
                    break;
                default:
                    // Do not change for other actions
                    // https://www.glfw.org/docs/latest/group__input.html#ga5bd751b27b90f865d2ea613533f0453c
                    break;
            }
            return;
        }

        // Only update state on press to avoid redundancy
        if (GLFW_PRESS != action) {
            return;
        }

        if (!altKeyPressed) {
            // Update VisualisationMode
            switch (keyCode) {
                case GLFW_KEY_1:
                    state->visualisationMode = state::VisualisationMode::pbr;
                    break;
                case GLFW_KEY_2:
                    state->visualisationMode = state::VisualisationMode::normal;
                    break;
                case GLFW_KEY_3:
                    state->visualisationMode = state::VisualisationMode::viewDirection;
                    break;
                case GLFW_KEY_4:
                    state->visualisationMode = state::VisualisationMode::lightDirection;
                    break;
                case GLFW_KEY_5:
                    state->visualisationMode = state::VisualisationMode::roughness;
                    break;
                case GLFW_KEY_6:
                    state->visualisationMode = state::VisualisationMode::metalness;
                    break;
                case GLFW_KEY_7:
                    state->visualisationMode = state::VisualisationMode::normalMap;
                    break;
                case GLFW_KEY_8:
                    state->visualisationMode = state::VisualisationMode::base;
                    break;
                default:
                    break;
            }
        } else {
            // Update PBRTerm
            switch (keyCode) {
                case GLFW_KEY_1:
                    state->pbrTerm = state::PBRTerm::all;
                    break;
                case GLFW_KEY_2:
                    state->pbrTerm = state::PBRTerm::ambient;
                    break;
                case GLFW_KEY_3:
                    state->pbrTerm = state::PBRTerm::diffuse;
                    break;
                case GLFW_KEY_4:
                    state->pbrTerm = state::PBRTerm::distribution;
                    break;
                case GLFW_KEY_5:
                    state->pbrTerm = state::PBRTerm::fresnel;
                    break;
                case GLFW_KEY_6:
                    state->pbrTerm = state::PBRTerm::geometry;
                    break;
                case GLFW_KEY_7:
                    state->pbrTerm = state::PBRTerm::specular;
                    break;
                default:
                    break;
            }
        }

        // Update shading details
        switch (keyCode) {
            case GLFW_KEY_N:
                state->detailsMask ^= static_cast<std::uint8_t>(state::ShadingDetails::normalMap);
                break;
            case GLFW_KEY_O:
                state->detailsMask ^= static_cast<std::uint8_t>(state::ShadingDetails::shadows);
                break;
            case GLFW_KEY_P:
                state->detailsMask ^= static_cast<std::uint8_t>(state::ShadingDetails::pcf);
                break;
            default:
                break;
        }

        // Update screen effects
        switch (keyCode) {
            case GLFW_KEY_T:
                state->toneMappingEnabled = !state->toneMappingEnabled;
                break;
            default:
                break;
        }

        // Camera callbacks
        switch (keyCode) {
            case GLFW_KEY_L:
                // Move the camera to the cfg::lightPosition
                state->camera2world = glm::translate(cfg::lightPosition) * cfg::cameraInitialRotation;
                break;
            case GLFW_KEY_I:
                // Reset camera to initial configuration
                state->camera2world = glm::translate(cfg::cameraInitialPosition) * cfg::cameraInitialRotation;
                break;
            default:
                break;
        }
    }

    void glfw_callback_key(GLFWwindow* window,
                           const int keyCode,
                           int /*scanCode*/,
                           const int action,
                           int /*modifierFlags*/) {
        const auto state = static_cast<state::State*>(glfwGetWindowUserPointer(window));
        assert(state);

        if (GLFW_KEY_ESCAPE == keyCode) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            return;
        }

        input_state_callback(state, keyCode, action);
        render_mode_callback(state, keyCode, action);
    }

    void glfw_callback_button(GLFWwindow* window, const int button, const int action, int) {
        const auto state = static_cast<state::State*>(glfwGetWindowUserPointer(window));
        assert(state);

        if (GLFW_MOUSE_BUTTON_RIGHT == button && GLFW_PRESS == action) {
            auto& flag = state->inputMap[static_cast<std::size_t>(state::InputState::mousing)];

            flag = !flag;
            if (flag) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            } else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }
    }

    void glfw_callback_motion(GLFWwindow* window, const double x, const double y) {
        const auto state = static_cast<state::State*>(glfwGetWindowUserPointer(window));
        assert(state);
        state->mouseX = static_cast<float>(x);
        state->mouseY = static_cast<float>(y);
    }

    void setup_window(const vkutils::VulkanWindow& window, state::State& state) {
        glfwSetWindowUserPointer(window.window, &state);
        glfwSetKeyCallback(window.window, &glfw::glfw_callback_key);
        glfwSetMouseButtonCallback(window.window, &glfw::glfw_callback_button);
        glfwSetCursorPosCallback(window.window, &glfw::glfw_callback_motion);
    }
}
