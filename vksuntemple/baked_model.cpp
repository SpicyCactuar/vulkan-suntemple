#include "baked_model.hpp"

#include <cstdio>
#include <cstring>

#include "../vkutils/error.hpp"

namespace baked {
    // See assets-bake/main.cpp for more info
    constexpr char kFileMagic[16] = "\0\0SPICYMESH";
    constexpr char kFileVariant[16] = "spicy";

    constexpr std::uint32_t kMaxString = 32 * 1024;

    void checked_read(FILE* input, const std::size_t bytes, void* buffer) {
        const auto ret = std::fread(buffer, 1, bytes, input);

        if (bytes != ret) {
            throw vkutils::Error("checked_read_(): expected %zu bytes, got %zu", bytes, ret);
        }
    }

    std::uint32_t read_uint32(FILE* input) {
        std::uint32_t ret;
        checked_read(input, sizeof(std::uint32_t), &ret);
        return ret;
    }

    std::string read_string(FILE* input) {
        const auto length = read_uint32(input);

        if (length >= kMaxString) {
            throw vkutils::Error("read_string_(): unexpectedly long string (%u bytes)", length);
        }

        std::string ret;
        ret.resize(length);

        checked_read(input, length, ret.data());
        return ret;
    }

    BakedModel load_baked_model_from_file(FILE* input, char const* inputName) {
        BakedModel bakedModel;

        // Figure out base path
        char const* pathBeg = inputName;
        char const* pathEnd = std::strrchr(pathBeg, '/');

        std::string const prefix = pathEnd ? std::string(pathBeg, pathEnd + 1) : "";

        // Read header and verify file magic and variant
        char magic[16];
        checked_read(input, 16, magic);

        if (0 != std::memcmp(magic, kFileMagic, 16)) {
            throw vkutils::Error("loadBakedModelFromFile(): %s: invalid file signature!", inputName);
        }

        char variant[16];
        checked_read(input, 16, variant);

        if (0 != std::memcmp(variant, kFileVariant, 16)) {
            throw vkutils::Error("loadBakedModelFromFile(): %s: file variant is '%s', expected '%s'", inputName,
                                 variant,
                                 kFileVariant);
        }

        // Read texture info
        const auto textureCount = read_uint32(input);
        for (std::uint32_t i = 0; i < textureCount; ++i) {
            const std::string name = read_string(input);
            std::uint8_t channels;
            checked_read(input, sizeof(std::uint8_t), &channels);

            BakedTextureInfo info{
                .path = prefix + name,
                .channels = channels
            };

            bakedModel.textures.emplace_back(std::move(info));
        }

        // Read material info
        const auto materialCount = read_uint32(input);
        for (std::uint32_t i = 0; i < materialCount; ++i) {
            BakedMaterialInfo info{
                .baseColorTextureId = read_uint32(input),
                .roughnessTextureId = read_uint32(input),
                .metalnessTextureId = read_uint32(input),
                .alphaMaskTextureId = read_uint32(input),
                .normalMapTextureId = read_uint32(input),
                .emissiveTextureId = read_uint32(input)
            };

            assert(info.baseColorTextureId < bakedModel.textures.size());
            assert(info.roughnessTextureId < bakedModel.textures.size());
            assert(info.metalnessTextureId < bakedModel.textures.size());
            assert(info.emissiveTextureId < bakedModel.textures.size());

            bakedModel.materials.emplace_back(std::move(info));
        }

        // Read mesh data
        const auto meshCount = read_uint32(input);
        for (std::uint32_t i = 0; i < meshCount; ++i) {
            BakedMeshData data;
            data.materialId = read_uint32(input);
            assert(data.materialId < bakedModel.materials.size());

            const auto V = read_uint32(input);
            const auto I = read_uint32(input);

            data.positions.resize(V);
            checked_read(input, V * sizeof(glm::vec3), data.positions.data());

            data.normals.resize(V);
            checked_read(input, V * sizeof(glm::vec3), data.normals.data());

            data.texcoords.resize(V);
            checked_read(input, V * sizeof(glm::vec2), data.texcoords.data());

            data.tangents.resize(V);
            checked_read(input, V * sizeof(glm::vec4), data.tangents.data());

            data.indices.resize(I);
            checked_read(input, I * sizeof(std::uint32_t), data.indices.data());

            bakedModel.meshes.emplace_back(std::move(data));
        }

        // Check
        char byte;
        const auto check = std::fread(&byte, 1, 1, input);

        if (0 != check) {
            std::fprintf(stderr, "Note: '%s' contains trailing bytes\n", inputName);
        }

        return bakedModel;
    }

    BakedModel load_baked_model(char const* modelPath) {
        FILE* modelFile = std::fopen(modelPath, "rb");
        if (!modelFile) {
            throw vkutils::Error("loadBakedModel(): unable to open '%s' for reading", modelPath);
        }

        try {
            auto ret = load_baked_model_from_file(modelFile, modelPath);
            std::fclose(modelFile);
            return ret;
        } catch (...) {
            std::fclose(modelFile);
            throw;
        }
    }
}
