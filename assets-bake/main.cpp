#include <iterator>
#include <vector>
#include <typeinfo>
#include <exception>
#include <filesystem>
#include <system_error>
#include <unordered_map>

#include <cstdio>
#include <cstring>

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "indexed_mesh.hpp"
#include "input_model.hpp"
#include "load_model_obj.hpp"

#include "../vkutils/error.hpp"

namespace {
    /*
     * File "magic". The first 16 bytes of our custom file are equal to this
     * magic value. This allows us to check whether a certain file is
     * (probably) of the right type. Having a file magic is relatively common
     * practice -- you can find a list of such magic sequences e.g. here:
     * https://en.wikipedia.org/wiki/List_of_file_signatures
     *
     * When picking a signature there are a few considerations. For example,
     * including non-printable characters (e.g. the \0) early keeps the file
     * from being misidentified as text.
     */
    constexpr char kFileMagic[16] = "\0\0SPICYMESH";
    constexpr char kFileVariant[16] = "spicy";

    /*
     * Fallback textures
     */
    constexpr char kTextureFallbackR1[] = "assets-src/r1.png";
    constexpr char kTextureFallbackRGBA1111[] = "assets-src/rgba1111.png";
    constexpr char kTextureFallbackRRGGB05051[] = "assets-src/rrggb05051.png";
    constexpr char kTextureFallbackRGB000[] = "assets-src/rgb000.png";

    struct TextureInfo {
        std::uint32_t uniqueId;
        std::uint8_t channels;
        std::string newPath;
    };

    void processModel(
        const char* inputObj,
        const char* output,
        const glm::mat4& transform = glm::identity<glm::mat4>()
    );

    InputModel normalize(InputModel);

    void writeModelData(
        FILE* out,
        const InputModel& model,
        const std::vector<IndexedMesh>& indexedMeshes,
        const std::unordered_map<std::string, TextureInfo>& textures);

    std::vector<IndexedMesh> indexMeshes(
        const InputModel& model,
        float errorTolerance = 1e-5f
    );

    std::unordered_map<std::string, TextureInfo> findUniqueTextures(
        const InputModel&);

    std::unordered_map<std::string, TextureInfo> populatePaths(
        std::unordered_map<std::string, TextureInfo>,
        const std::filesystem::path& textureDir
    );
}


int main() try {
#	if !defined(NDEBUG)
    std::printf("Suggest running this in release mode (it appears to be running in debug)\n");
    /*
     * A few notes:
     *
     * - ZStd benefits immensly from compiler optimizations.
     *
     * - NDEBUG is the standard macro to control the behaviour of assert(). When
     *   NDEBUG is defined, assert() will "do nothing" (they're expanded to an
     *   empty statement). This is typically desirable in a release build, but not
     *   necessary or guaranteed. (Indeed, the premake sets NDEBUG explicitly for
     *   this project -- this is why the check above works. But don't rely on this
     *   blindly.)
     * 
     * The binary .spicymesh should be unchanged between debug and release
     * builds, so you can safely use the release build to create the file once,
     * even while debugging the main CW3 program.
     */
#	endif
    processModel(
        "assets-src/suntemple.obj-zstd",
        "assets/suntemple.spicymesh"
    );

    return 0;
} catch (const std::exception& e) {
    std::fprintf(stderr, "Top-level exception [%s]:\n%s\nExiting.\n", typeid(e).name(), e.what());
    return 1;
}

namespace {
    void processModel(const char* inputObj, const char* output, const glm::mat4& transform) {
        static constexpr std::size_t vertexSize = sizeof(float) * (3 + 3 + 2);

        // Figure out output paths
        const std::filesystem::path outname(output);
        const std::filesystem::path rootdir = outname.parent_path();
        const std::filesystem::path basename = outname.stem();
        const std::filesystem::path textureDir = basename.string() + "-tex";

        // Load input model
        const auto model = normalize(loadCompressedObj(inputObj));

        std::size_t inputVerts = 0;
        for (const auto& mesh : model.meshes) {
            inputVerts += mesh.vertexCount;
        }

        std::printf("%s: %zu meshes, %zu materials\n", inputObj, model.meshes.size(), model.materials.size());
        std::printf(" - triangle soup vertices: %zu => %zu kB\n", inputVerts, inputVerts * vertexSize / 1024);

        // Index meshes
        const auto indexed = indexMeshes(model);

        std::size_t outputVerts = 0, outputIndices = 0;
        for (const auto& mesh : indexed) {
            outputVerts += mesh.vertices.size();
            outputIndices += mesh.indices.size();
        }

        std::printf(" - indexed vertices: %zu with %zu indices => %zu kB\n", outputVerts, outputIndices,
                    (outputVerts * vertexSize + outputIndices * sizeof(std::uint32_t)) / 1024);

        // Find list of unique textures
        const auto textures = populatePaths(findUniqueTextures(model), textureDir);

        std::printf(" - unique textures: %zu\n", textures.size());

        // Ensure output directory exists
        std::filesystem::create_directories(rootdir);

        // Output mesh data
        auto mainpath = rootdir / basename;
        mainpath.replace_extension("spicymesh");

        FILE* fof = std::fopen(mainpath.string().c_str(), "wb");
        if (!fof)
            throw vkutils::Error("Unable to open '%s' for writing", mainpath.string().c_str());

        try {
            writeModelData(fof, model, indexed, textures);
        } catch (...) {
            std::fclose(fof);
            throw;
        }

        std::fclose(fof);

        // Copy textures
        std::filesystem::create_directories(rootdir / textureDir);

        std::size_t errors = 0;
        for (const auto& textureEntry : textures) {
            const auto dest = rootdir / textureEntry.second.newPath;

            std::error_code errorCode;
            bool ret = std::filesystem::copy_file(
                textureEntry.first,
                dest,
                std::filesystem::copy_options::none,
                errorCode
            );

            if (!ret) {
                ++errors;
                std::fprintf(stderr, "copy_file(): '%s' failed: %s (%s)\n", dest.string().c_str(),
                             errorCode.message().c_str(),
                             errorCode.category().name());
            }
        }

        const auto total = textures.size();
        std::printf("Copied %zu textures out of %zu.\n", total - errors, total);
        if (errors) {
            std::fprintf(
                stderr,
                "Some copies reported an error. Currently, the code will never overwrite existing files. The errors likely just indicate that the file was copied previously. Remove old files manually, if necessary.\n");
        }
    }
}

namespace {
    InputModel normalize(InputModel model) {
        for (auto& material : model.materials) {
            if (material.baseColorTexturePath.empty()) {
                material.baseColorTexturePath = kTextureFallbackRGBA1111;
            }
            if (material.roughnessTexturePath.empty()) {
                material.roughnessTexturePath = kTextureFallbackR1;
            }
            if (material.metalnessTexturePath.empty()) {
                material.metalnessTexturePath = kTextureFallbackR1;
            }
            if (material.normalMapTexturePath.empty()) {
                material.normalMapTexturePath = kTextureFallbackRRGGB05051;
            }
            if (material.emissiveTexturePath.empty()) {
                material.emissiveTexturePath = kTextureFallbackRGB000;
            }
        }

        // This should use the move constructor implicitly
        return model;
    }
}

namespace {
    void checkedWrite(FILE* out, const std::size_t bytes, const void* data) {
        if (const auto ret = std::fwrite(data, 1, bytes, out);
            ret != bytes) {
            throw vkutils::Error("fwrite() failed: %zu instead of %zu", ret, bytes);
        }
    }

    void writeString(FILE* out, const char* string) {
        // Write a string
        // Format:
        //  - uint32_t : N = length of string in bytes, including terminating '\0'
        //  - N x char : string
        const std::uint32_t length = static_cast<std::uint32_t>(std::strlen(string) + 1);
        checkedWrite(out, sizeof(std::uint32_t), &length);

        checkedWrite(out, length, string);
    }

    void writeModelData(FILE* out,
                        const InputModel& model,
                        const std::vector<IndexedMesh>& indexedMeshes,
                        const std::unordered_map<std::string, TextureInfo>& textures) {
        // Write header
        // Format:
        //   - char[16] : file magic
        //   - char[16] : file variant ID
        checkedWrite(out, sizeof(char) * 16, kFileMagic);
        checkedWrite(out, sizeof(char) * 16, kFileVariant);

        // Write list of unique textures
        // Format:
        //  - unit32_t : U = number of unique textures
        //  - repeat U times:
        //    - string : path to texture
        //    - uint8_t : number of channels in texture
        std::vector<const TextureInfo*> orderedUnique(textures.size());
        for (const auto& texture : textures) {
            assert(!orderedUnique[texture.second.uniqueId]);
            orderedUnique[texture.second.uniqueId] = &texture.second;
        }

        std::uint32_t const textureCount = static_cast<std::uint32_t>(orderedUnique.size());
        checkedWrite(out, sizeof(textureCount), &textureCount);

        for (const auto& tex : orderedUnique) {
            assert(tex);
            writeString(out, tex->newPath.c_str());

            std::uint8_t channels = tex->channels;
            checkedWrite(out, sizeof(channels), &channels);
        }

        // Write material information
        // Format:
        //  - uint32_t : M = number of materials
        //  - repeat M times:
        //    - uin32_t : base color texture index
        //    - uin32_t : roughness texture index
        //    - uin32_t : metalness texture index
        //    - uin32_t : alphaMask texture index (or 0xffffffff if none)
        //    - uin32_t : normalMap texture index (or 0xffffffff if none)
        //    - uin32_t : emissive texture index
        const std::uint32_t materialCount = static_cast<std::uint32_t>(model.materials.size());
        checkedWrite(out, sizeof(materialCount), &materialCount);

        for (const auto& material : model.materials) {
            const auto writeTex = [&](const std::string& rawTexturePath) {
                if (rawTexturePath.empty()) {
                    static constexpr std::uint32_t sentinel = ~static_cast<std::uint32_t>(0);
                    checkedWrite(out, sizeof(std::uint32_t), &sentinel);
                    return;
                }

                const auto it = textures.find(rawTexturePath);
                assert(textures.end() != it);

                checkedWrite(out, sizeof(std::uint32_t), &it->second.uniqueId);
            };

            writeTex(material.baseColorTexturePath);
            writeTex(material.roughnessTexturePath);
            writeTex(material.metalnessTexturePath);
            writeTex(material.alphaMaskTexturePath);
            writeTex(material.normalMapTexturePath);
            writeTex(material.emissiveTexturePath);
        }

        // Write mesh data
        // Format:
        //  - uint32_t : M = number of meshes
        //  - repeat M times:
        //    - uint32_t : material index
        //    - uint32_t : V = number of vertices
        //    - uint32_t : I = number of indices
        //    - repeat V times: vec3 position
        //    - repeat V times: vec3 normal
        //    - repeat V times: vec2 texture coordinate
        //    - repeat I times: uint32_t index
        const std::uint32_t meshCount = static_cast<std::uint32_t>(model.meshes.size());
        checkedWrite(out, sizeof(meshCount), &meshCount);

        assert(model.meshes.size() == indexedMeshes.size());
        for (std::size_t i = 0; i < model.meshes.size(); ++i) {
            const auto& modelMesh = model.meshes[i];

            std::uint32_t materialIndex = static_cast<std::uint32_t>(modelMesh.materialIndex);
            checkedWrite(out, sizeof(materialIndex), &materialIndex);

            const auto& indexedMesh = indexedMeshes[i];

            std::uint32_t vertexCount = static_cast<std::uint32_t>(indexedMesh.vertices.size());
            checkedWrite(out, sizeof(vertexCount), &vertexCount);
            std::uint32_t indexCount = static_cast<std::uint32_t>(indexedMesh.indices.size());
            checkedWrite(out, sizeof(indexCount), &indexCount);

            checkedWrite(out, sizeof(glm::vec3) * vertexCount, indexedMesh.vertices.data());
            checkedWrite(out, sizeof(glm::vec3) * vertexCount, indexedMesh.normals.data());
            checkedWrite(out, sizeof(glm::vec2) * vertexCount, indexedMesh.texCoordinates.data());
            checkedWrite(out, sizeof(glm::vec4) * vertexCount, indexedMesh.tangent.data());

            checkedWrite(out, sizeof(std::uint32_t) * indexCount, indexedMesh.indices.data());
        }
    }
}

namespace {
    std::vector<IndexedMesh> indexMeshes(const InputModel& model, float errorTolerance) {
        std::vector<IndexedMesh> indexed;

        for (const auto& mesh : model.meshes) {
            const auto endIndex = mesh.vertexStartIndex + mesh.vertexCount;

            TriangleSoup soup;

            soup.vertices.reserve(mesh.vertexCount);
            for (std::size_t i = mesh.vertexStartIndex; i < endIndex; ++i) {
                soup.vertices.emplace_back(model.positions[i]);
            }

            soup.texCoordinates.reserve(mesh.vertexCount);
            for (std::size_t i = mesh.vertexStartIndex; i < endIndex; ++i) {
                soup.texCoordinates.emplace_back(model.texCoordinates[i]);
            }

            soup.normals.reserve(mesh.vertexCount);
            for (std::size_t i = mesh.vertexStartIndex; i < endIndex; ++i) {
                soup.normals.emplace_back(model.normals[i]);
            }

            indexed.emplace_back(makeIndexedMesh(soup, errorTolerance));
        }

        return indexed;
    }
}

namespace {
    std::unordered_map<std::string, TextureInfo> findUniqueTextures(const InputModel& model) {
        std::unordered_map<std::string, TextureInfo> unique;

        std::uint32_t textureId = 0;
        const auto addUnique = [&](const std::string& path, const std::uint8_t channels) {
            if (path.empty()) {
                return;
            }

            const TextureInfo info{
                .uniqueId = textureId,
                .channels = channels
            };

            const auto [_, isNew] = unique.emplace(std::make_pair(path, info));

            if (isNew) {
                ++textureId;
            }
        };

        for (const auto& material : model.materials) {
            addUnique(material.baseColorTexturePath, 4);
            addUnique(material.roughnessTexturePath, 1);
            addUnique(material.metalnessTexturePath, 1);
            addUnique(material.alphaMaskTexturePath, 4); // assume == baseColor
            addUnique(material.normalMapTexturePath, 3); // xyz only
            addUnique(material.emissiveTexturePath, 4);
        }

        return unique;
    }

    std::unordered_map<std::string, TextureInfo> populatePaths(std::unordered_map<std::string, TextureInfo> textures,
                                                               const std::filesystem::path& textureDir) {
        for (auto& entry : textures) {
            const std::filesystem::path originalPath(entry.first);
            const auto filename = originalPath.filename();
            const auto newPath = textureDir / filename;

            auto& textureInfo = entry.second;
            textureInfo.newPath = newPath.string();
        }

        return textures;
    }
}
