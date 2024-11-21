#include "load_model_obj.hpp"

#include <unordered_set>

#include <cassert>
#include <cstring>

#include <rapidobj/rapidobj.hpp>

#include "input_model.hpp"
#include "zstdistream.hpp"

#include "../vkutils/error.hpp"

InputModel loadCompressedObj(const char* rawPath) {
    assert(rawPath);

    // Ask rapidobj to load the requested file
    rapidobj::MaterialLibrary const mlib = rapidobj::MaterialLibrary::SearchPath(
        std::filesystem::absolute(std::filesystem::path(rawPath).remove_filename()));

    ZStdIStream ins(rawPath);
    auto result = rapidobj::ParseStream(ins, mlib);
    if (result.error) {
        throw vkutils::Error("Unable to load OBJ file '%s': %s", rawPath, result.error.code.message().c_str());
    }

    // OBJ files can define faces that are not triangles. However, Vulkan will
    // only render triangles (or lines and points), so we must triangulate any
    // faces that are not already triangles. Fortunately, rapidobj can do this
    // for us.
    rapidobj::Triangulate(result);

    // Find the path to the OBJ file
    const char* pathBeg = rawPath;
    const char* pathEnd = std::strrchr(pathBeg, '/');

    std::string const prefix = pathEnd ? std::string(pathBeg, pathEnd + 1) : "";

    // Convert the OBJ data into a InputModel structure.
    // First, extract material data.
    InputModel loadedModel;

    loadedModel.modelSourcePath = rawPath;

    for (const auto& material : result.materials) {
        InputMaterialInfo materialInfo;

        materialInfo.materialName = material.name;

        materialInfo.baseColor = glm::vec3(material.diffuse[0], material.diffuse[1], material.diffuse[2]);

        materialInfo.baseRoughness = material.roughness;
        materialInfo.baseMetalness = material.metallic;

        if (!material.diffuse_texname.empty()) {
            materialInfo.baseColorTexturePath = prefix + material.diffuse_texname;
        }
        if (!material.roughness_texname.empty()) {
            materialInfo.roughnessTexturePath = prefix + material.roughness_texname;
        }
        if (!material.metallic_texname.empty()) {
            materialInfo.metalnessTexturePath = prefix + material.metallic_texname;
        }
        if (!material.alpha_texname.empty()) {
            materialInfo.alphaMaskTexturePath = prefix + material.alpha_texname;
        }
        if (!material.normal_texname.empty()) {
            materialInfo.normalMapTexturePath = prefix + material.normal_texname;
        }
        if (!material.emissive_texname.empty()) {
            materialInfo.emissiveTexturePath = prefix + material.emissive_texname;
        }

        loadedModel.materials.emplace_back(std::move(materialInfo));
    }

    // Next, extract the actual mesh data. There are some complications:
    // - OBJ use separate indices to positions, normals and texture coords. To
    //   deal with this, the mesh is turned into an unindexed triangle soup.
    // - OBJ uses three methods of grouping faces:
    //   - 'o' = object
    //   - 'g' = group
    //   - 'usemtl' = switch materials
    //  The first two create logical objects/groups. The latter switches
    //  materials. We want to primarily group faces by material (and possibly
    //  secondarily by other logical groupings). 
    //
    // RapidOBJ exposes a per-face material index.
    std::unordered_set<std::size_t> activeMaterials;
    for (const auto& shape : result.shapes) {
        const auto& shapeName = shape.name;

        // Scan shape for materials
        activeMaterials.clear();

        for (std::size_t i = 0; i < shape.mesh.indices.size(); ++i) {
            // Always triangles; see Triangulate() above
            const auto faceId = i / 3;

            assert(faceId < shape.mesh.material_ids.size());
            const auto matId = shape.mesh.material_ids[faceId];

            assert(matId < static_cast<int>(loadedModel.materials.size()));
            activeMaterials.emplace(matId);
        }

        // Process vertices for active material
        // This does multiple passes over the vertex data, which is less than
        // optimal...
        //
        // Note: we still keep different "shapes" separate. For static meshes,
        // one could merge all vertices with the same material for a bit more
        // efficient rendering.
        for (const auto materialId : activeMaterials) {
            // Keep track of mesh names; this can be useful for debugging.
            std::string meshName;
            if (1 == activeMaterials.size()) {
                meshName = shapeName;
            } else {
                meshName = shapeName + "::" + loadedModel.materials[materialId].materialName;
            }

            // Extract this material's vertices.
            const auto firstVertex = loadedModel.positions.size();

            for (std::size_t i = 0; i < shape.mesh.indices.size(); ++i) {
                const auto faceId = i / 3; // Always triangles; see Triangulate() above
                const auto faceMaterial = static_cast<std::size_t>(shape.mesh.material_ids[faceId]);

                if (faceMaterial != materialId) {
                    continue;
                }

                const auto& index = shape.mesh.indices[i];

                loadedModel.positions.emplace_back(
                    result.attributes.positions[index.position_index * 3 + 0],
                    result.attributes.positions[index.position_index * 3 + 1],
                    result.attributes.positions[index.position_index * 3 + 2]
                );

                loadedModel.texCoordinates.emplace_back(
                    result.attributes.texcoords[index.texcoord_index * 2 + 0],
                    result.attributes.texcoords[index.texcoord_index * 2 + 1]
                );

                loadedModel.normals.emplace_back(
                    result.attributes.normals[index.normal_index * 3 + 0],
                    result.attributes.normals[index.normal_index * 3 + 1],
                    result.attributes.normals[index.normal_index * 3 + 2]
                );
            }

            const auto vertexCount = loadedModel.positions.size() - firstVertex;

            loadedModel.meshes.emplace_back(InputMeshInfo{
                std::move(meshName),
                materialId,
                firstVertex,
                vertexCount
            });
        }
    }

    return loadedModel;
}
