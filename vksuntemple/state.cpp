#include "state.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include "config.hpp"

namespace state {
    void update_state(State& state, const float elapsedTime) {
        auto& cam = state.camera2world;

        if (state.inputMap[static_cast<std::size_t>(InputState::mousing)]) {
            // Only update the rotation on the second frame of mouse navigation. This ensures that the previousX
            // and Y variables are initialized to sensible values.
            if (state.wasMousing) {
                constexpr auto mouseSensitivity = cfg::cameraMouseSensitivity;
                const auto dx = mouseSensitivity * (state.mouseX - state.previousX);
                const auto dy = mouseSensitivity * (state.mouseY - state.previousY);

                cam = cam * glm::rotate(-dy, glm::vec3(1.f, 0.f, 0.f));
                cam = cam * glm::rotate(-dx, glm::vec3(0.f, 1.f, 0.f));
            }

            state.previousX = state.mouseX;
            state.previousY = state.mouseY;
            state.wasMousing = true;
        } else {
            state.wasMousing = false;
        }

        const auto move = elapsedTime * cfg::cameraBaseSpeed *
                          (state.inputMap[static_cast<std::size_t>(InputState::fast)] ? cfg::cameraFastMult : 1.f) *
                          (state.inputMap[static_cast<std::size_t>(InputState::slow)] ? cfg::cameraSlowMult : 1.f);

        if (state.inputMap[static_cast<std::size_t>(InputState::forward)]) {
            cam = cam * glm::translate(glm::vec3(0.f, 0.f, -move));
        }
        if (state.inputMap[static_cast<std::size_t>(InputState::backward)]) {
            cam = cam * glm::translate(glm::vec3(0.f, 0.f, +move));
        }

        if (state.inputMap[static_cast<std::size_t>(InputState::strafeLeft)]) {
            cam = cam * glm::translate(glm::vec3(-move, 0.f, 0.f));
        }
        if (state.inputMap[static_cast<std::size_t>(InputState::strafeRight)]) {
            cam = cam * glm::translate(glm::vec3(+move, 0.f, 0.f));
        }

        if (state.inputMap[static_cast<std::size_t>(InputState::levitate)]) {
            cam = cam * glm::translate(glm::vec3(0.f, +move, 0.f));
        }
        if (state.inputMap[static_cast<std::size_t>(InputState::sink)]) {
            cam = cam * glm::translate(glm::vec3(0.f, -move, 0.f));
        }
    }

    glm::vec3 State::cameraPosition() const {
        return {camera2world[3][0], camera2world[3][1], camera2world[3][2]};
    }
}
