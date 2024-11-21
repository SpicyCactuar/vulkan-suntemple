#include "baked_model.hpp"

#include <cstdio>
#include <cstring>

#include "../vkutils/error.hpp"

namespace baked {
    // See assets-bake/main.cpp for more info
    constexpr char kFileMagic[16] = "\0\0SPICYMESH";
    constexpr char kFileVariant[16] = "spicy";

    constexpr std::uint32_t kMaxString = 32 * 1024;

    void checkedRead(FILE* input, const std::size_t bytes, void* buffer) {
        const auto ret = std::fread(buffer, 1, bytes, input);

        if (bytes != ret) {
            throw vkutils::Error("checked_read_(): expected %zu bytes, got %zu", bytes, ret);
        }
    }

    std::uint32_t readUint32(FILE* input) {
        std::uint32_t ret;
        checkedRead(input, sizeof(std::uint32_t), &ret);
        return ret;
    }

    std::string readString(FILE* input) {
        const auto length = readUint32(input);

        if (length >= kMaxString) {
            throw vkutils::Error("read_string_(): unexpectedly long string (%u bytes)", length);
        }

        std::string ret;
        ret.resize(length);

        checkedRead(input, length, ret.data());
        return ret;
    }

    BakedModel loadBakedModelFromFile(FILE* input, char const* inputName) {
        BakedModel bakedModel;

        // Figure out base path
        char const* pathBeg = inputName;
        char const* pathEnd = std::strrchr(pathBeg, '/');

        std::string const prefix = pathEnd ? std::string(pathBeg, pathEnd + 1) : "";

        // Read header and verify file magic and variant
        char magic[16];
        checkedRead(input, 16, magic);

        if (0 != std::memcmp(magic, kFileMagic, 16)) {
            throw vkutils::Error("loadBakedModelFromFile(): %s: invalid file signature!", inputName);
        }

        char variant[16];
        checkedRead(input, 16, variant);

        if (0 != std::memcmp(variant, kFileVariant, 16)) {
            throw vkutils::Error("loadBakedModelFromFile(): %s: file variant is '%s', expected '%s'", inputName,
                                 variant,
                                 kFileVariant);
        }

        // Read texture info
        const auto textureCount = readUint32(input);
        for (std::uint32_t i = 0; i < textureCount; ++i) {
            const std::string name = readString(input);
            std::uint8_t channels;
            checkedRead(input, sizeof(std::uint8_t), &channels);

            BakedTextureInfo info{
                .path = prefix + name,
                .channels = channels
            };

            bakedModel.textures.emplace_back(std::move(info));
        }

        // Read material info
        const auto materialCount = readUint32(input);
        for (std::uint32_t i = 0; i < materialCount; ++i) {
            BakedMaterialInfo info{
                .baseColorTextureId = readUint32(input),
                .roughnessTextureId = readUint32(input),
                .metalnessTextureId = readUint32(input),
                .alphaMaskTextureId = readUint32(input),
                .normalMapTextureId = readUint32(input),
                .emissiveTextureId = readUint32(input)
            };

            assert(info.baseColorTextureId < bakedModel.textures.size());
            assert(info.roughnessTextureId < bakedModel.textures.size());
            assert(info.metalnessTextureId < bakedModel.textures.size());
            assert(info.emissiveTextureId < bakedModel.textures.size());

            bakedModel.materials.emplace_back(std::move(info));
        }

        // Read mesh data
        const auto meshCount = readUint32(input);
        for (std::uint32_t i = 0; i < meshCount; ++i) {
            BakedMeshData data;
            data.materialId = readUint32(input);
            assert(data.materialId < bakedModel.materials.size());

            const auto V = readUint32(input);
            const auto I = readUint32(input);

            data.positions.resize(V);
            checkedRead(input, V * sizeof(glm::vec3), data.positions.data());

            data.normals.resize(V);
            checkedRead(input, V * sizeof(glm::vec3), data.normals.data());

            data.texcoords.resize(V);
            checkedRead(input, V * sizeof(glm::vec2), data.texcoords.data());

            data.tangents.resize(V);
            checkedRead(input, V * sizeof(glm::vec4), data.tangents.data());

            data.indices.resize(I);
            checkedRead(input, I * sizeof(std::uint32_t), data.indices.data());

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

    BakedModel loadBakedModel(char const* modelPath) {
        FILE* modelFile = std::fopen(modelPath, "rb");
        if (!modelFile) {
            throw vkutils::Error("loadBakedModel(): unable to open '%s' for reading", modelPath);
        }

        try {
            auto ret = loadBakedModelFromFile(modelFile, modelPath);
            std::fclose(modelFile);
            return ret;
        } catch (...) {
            std::fclose(modelFile);
            throw;
        }
    }
}
