//☀Rise☀
#include "Rise/shader.h"

#include "Rise/logger.h"
#include "Rise/loader.h"
#include "Rise/rise.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <shaderc/shaderc.hpp>

#include <fstream>
#include <array>

namespace Rise {

    std::vector<uint32_t> CompileFile(Logger& logger, const std::string& source_name,
        shaderc_shader_kind kind,
        const char* source,
        size_t sourceSize,
        bool optimize = false) {
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;

        if (optimize) options.SetOptimizationLevel(shaderc_optimization_level_size);

        shaderc::SpvCompilationResult module =
            compiler.CompileGlslToSpv(source, sourceSize, kind, source_name.c_str(), options);

        if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
            std::string error = module.GetErrorMessage();
            logger.Error(error);
        }

        return { module.cbegin(), module.cend() };
    }

    void IShader::Meta::Setup(const Data& data) {
        isVertex = data["type"].as<std::string>() == "vertex";
        
        {
            std::unordered_map<std::string, uint32_t> names;
            std::vector<BaseType> types;

            for (auto pair : data["constant"]) {
                std::string name = pair[0];
                std::string typeStr = pair[1];
                names.try_emplace(name, static_cast<uint32_t>(names.size()));
                types.emplace_back(BaseType::FromString(typeStr.c_str()));
            }

            constant = { names, types };
        }

        constantOffset = data["constantOffset"];

        {
            uniforms.clear();

            for (auto uniform : data["uniforms"]) {
                uint16_t set = uniform["set"];
                uint16_t binding = uniform["binding"];

                auto& uniformData = uniforms.try_emplace(CombinedKey<uint16_t, uint16_t>(set, binding)).first->second;

                uniformData.count = uniform["count"].as<uint32_t>(1);

                auto format = uniform["format"];

                if (format.isString()) {
                    auto sampler2d = format.as<std::string>() == "sampler2d";
                    uniformData.texture2d = sampler2d || format.as<std::string>() == "texture2d";
                    uniformData.sampler = sampler2d || format.as<std::string>() == "sampler";
                }
                else if (format.isArray()) {
                    std::unordered_map<std::string, uint32_t> names;
                    std::vector<BaseType> types;

                    for (auto pair : format) {
                        std::string name = pair[0];
                        std::string typeStr = pair[1];
                        names.try_emplace(name, static_cast<uint32_t>(names.size()));
                        types.emplace_back(BaseType::FromString(typeStr.c_str()));
                    }

                    uniformData.metadata = { names, types };
                }

            }
        }
    }

    uint32_t IShader::Meta::MaxSetNumber() const {
        uint32_t max = 0;

        for (auto& pair : uniforms) {
            uint32_t setNumber = pair.first.Get<0>() + 1u;
            if (setNumber > max) {
                max = setNumber;
            }
        }

        return max;
    }

    void IShader::Meta::PopulateLayoutBindings(Renderer::SetLayout& layout, std::vector<VkDescriptorSetLayoutBinding>& container, uint32_t set) const {
        for (const auto& uniformPair : uniforms) {
            if (set != uniformPair.first.Get<0>()) {
                continue;
            }

            auto& binding = container.emplace_back();
            binding.binding = uniformPair.first.Get<1>();

            auto& uniformData = uniformPair.second;

            if (uniformData.metadata.SizeOf() > 0) {
                binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            }
            else if (uniformData.texture2d) {
                if (uniformData.sampler) {
                    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                }
                else {
                    binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                }
            }
            else {
                if (uniformData.sampler) {
                    binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
                }
            }

            ++layout._types[binding.descriptorType];

            binding.descriptorCount = uniformData.count;

            VkShaderStageFlags stage = isVertex ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT;

            binding.stageFlags = stage;
            binding.pImmutableSamplers = nullptr; // Optional
        }
    }

    void IShader::LoadFromFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        _name = filename;

        if (!file.is_open()) {
            Error("failed to open file!");
        }

        size_t fileSize = (size_t)file.tellg();
        size_t bufferSize = (fileSize + sizeof(uint32_t) - 1) / sizeof(uint32_t);
        _data.resize(bufferSize);

        file.seekg(0);
        file.read(reinterpret_cast<char*>(_data.data()), fileSize);

        file.close();

        if (_data[0] != 0x7230203) {
            shaderc_shader_kind type;

            if (_meta->isVertex) {
                type = shaderc_vertex_shader;
            }
            else {
                type = shaderc_fragment_shader;
            }
            _data = std::move(CompileFile(Instance()->Logger(), filename, type, reinterpret_cast<char*>(_data.data()), fileSize, /* optimize = */ false));
        }

        MarkLoaded();
    }

}
