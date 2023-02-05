//☀Rise☀
#ifndef shader_h
#define shader_h

#include "resource.h"
#include "utils.h"
#include "render_pass.h"
#include "image.h"

#include <vulkan/vulkan.h>

#include <vector>
#include <future>

namespace Rise {

    class Window;

    class IShader : public ResourceBase, public RiseObject {
    public:

        using Ptr = std::shared_ptr<IShader>;
        constexpr static const char* Ext = "shdr";

        class Meta {
        public:
            bool isVertex;
            NamedMetadata<BaseType> constant;
            uint32_t constantOffset;

            struct UniformData {
                NamedMetadata<BaseType> metadata;
                uint32_t count;
                bool texture2d = false;
                bool sampler = false;
            };

            std::unordered_map<CombinedKey<uint16_t, uint16_t>, UniformData> uniforms;

            void Setup(const Data& data);

            uint32_t MaxSetNumber() const;

            void PopulateLayoutBindings(Renderer::SetLayout& layout, std::vector<VkDescriptorSetLayoutBinding>& container, uint32_t set) const;
        };

        IShader(Core* core, Meta* meta)
            : RiseObject(core), _meta(meta) {}

        const Meta* meta() {
            return _meta;
        }

        void LoadFromFile(const std::string& filename);

        class OneTimeModule {
        public:
            OneTimeModule(const IShader& shader) 
                : _shader(&shader) {
                VkShaderModuleCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
                createInfo.codeSize = shader._data.size() * sizeof(*shader._data.begin());
                createInfo.pCode = shader._data.data();

                {
                    std::lock_guard<std::recursive_mutex> lg(Rise::Instance()->_deviceLock);
                    if (vkCreateShaderModule(Rise::Instance()->_vDevice, &createInfo, nullptr, &_value) != VK_SUCCESS) {
                        shader.Error("failed to create shader module!");
                    }
                }
            }
            ~OneTimeModule() {
                std::lock_guard<std::recursive_mutex> lg(Rise::Instance()->_deviceLock);
                vkDestroyShaderModule(Rise::Instance()->_vDevice, VShaderModule(), nullptr);
            }

            const VkShaderModule& VShaderModule() {
                return _value;
            }

        private:
            const IShader* _shader = nullptr;
            VkShaderModule _value = nullptr;
        };

        OneTimeModule createModule() const {
            return OneTimeModule(*this);
        }

    private:

        std::string _name;
        Meta* _meta;
        std::vector<uint32_t> _data;
    };

}

#endif /* shader_h */
