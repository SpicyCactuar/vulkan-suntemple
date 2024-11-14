#pragma once

#include <glm/gtx/transform.hpp>

#include "config.hpp"

namespace state {
    enum class InputState {
        forward,
        backward,
        strafeLeft,
        strafeRight,
        levitate,
        sink,
        fast,
        slow,
        mousing,
        max
    };

    /*
     * base = 1 - Base Colour as RGB (default)
     * normal = 2 - Normals as RGB
     * viewDirection = 3 - Direction of fragment to camera
     * lightDirection = 4 - Direction of fragment to light
     * roughness = 5 - Roughness as RGB
     * metalness = 6 - Metalness as RGB
     * normalMap = 7 - Normal Map as RGB
     * pbr = 8 - PBR shading
     */
    enum class VisualisationMode {
        pbr = 1,
        normal = 2,
        viewDirection = 3,
        lightDirection = 4,
        roughness = 5,
        metalness = 6,
        normalMap = 7,
        base = 8
    };

    /*
     * PBR terms only visible when VisualisationMode.pbr == state.visualisationMode
     *
     * all = 1 - Full PBR equation (default)
     * ambient = 2 - Ambient term (L_ambient)
     * diffuse = 3 - Diffuse term (L_diffuse)
     * distribution = 4 - Normal distribution term (D)
     * fresnel = 5 - Fresnel term (F)
     * geometry = 6 - Geometry attenuation term (G)
     * specular = 7 - Specular PBR term ((D * F * G) / (4 * n.v * n.l))
     */
    enum class PBRTerm {
        all = 1,
        ambient = 2,
        diffuse = 3,
        distribution = 4,
        fresnel = 5,
        geometry = 6,
        specular = 7
    };

    /*
     * Allows toggling different shading detailing effects.
     * Represented as bit mask to allow toggling independently.
     *
     * none = 0x00 - No details enabled
     * normalMap = 0x01 - Toggles normal mapping in opaque|alpha_mask.frag
     * shadows = 0x02 - Toggles shadow shading, note that shadow mapping is performed anyway
     * pcf = 0x04 - Toggles PCF for shadow shading, if shadows is not enabled this has no effect
     */
    enum class ShadingDetails : std::uint8_t {
        none = 0x00,
        normalMap = 0x01,
        shadows = 0x02,
        pcf = 0x04
    };

    struct State {
        bool inputMap[std::size_t(InputState::max)] = {};

        float mouseX = 0.f, mouseY = 0.f;
        float previousX = 0.f, previousY = 0.f;

        bool wasMousing = false;

        glm::mat4 camera2world = glm::translate(cfg::cameraInitialPosition) * cfg::cameraInitialRotation;

        VisualisationMode visualisationMode = VisualisationMode::pbr;

        PBRTerm pbrTerm = PBRTerm::all;
        std::uint8_t detailsMask = static_cast<std::uint8_t>(ShadingDetails::normalMap);

        bool toneMappingEnabled = false;

        glm::vec3 cameraPosition() const;
    };

    void update_state(State& state, float elapsedTime);
}
