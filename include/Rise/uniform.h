//☀Rise☀
#ifndef uniform_h
#define uniform_h

#include "render_pass.h"
#include "resource.h"
#include "gpu_allocator.h"
#include "utils.h"

#include <vulkan/vulkan.h>


namespace Rise {

    class UniformSetter {
    public:
        virtual ~UniformSetter() = default;

        virtual bool Ready() { return true; }
    };

    class BufferUniformSetter : public UniformSetter {
        friend Window;

    public:
        static constexpr VkDescriptorType DescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

        explicit BufferUniformSetter(VkDescriptorSet vDescriptorSet, uint32_t binding, const std::vector<uint8_t>& data) {
            Init(vDescriptorSet, binding, data.size());
            UpdateUniformBuffer(data);
        };
        virtual ~BufferUniformSetter() {
            Destroy();
        }

    private:

        void Init(VkDescriptorSet vDescriptorSet, uint32_t binding, VkDeviceSize size);
        void Destroy();

        void InitBuffer(VkDeviceSize size);
        void UpdateUniformBuffer(const std::vector<uint8_t>& data);

        GpuAllocator::Buffer uniformBuffer;
    };

    class IImage;
    class Sampler;

    class ImageUniformSetter : public UniformSetter {
        friend Window;

    public:
        static constexpr VkDescriptorType DescriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

        explicit ImageUniformSetter(VkDescriptorSet vDescriptorSet, uint32_t binding, 
            std::shared_ptr<IImage> image, std::shared_ptr<Sampler> sampler)
        : _image(image), _sampler(sampler) {
            Init(vDescriptorSet, binding);
        };
        virtual ~ImageUniformSetter() {}

        bool Ready() override {
            return _image && _image->IsLoaded();
        }

    private:

        void Init(VkDescriptorSet vDescriptorSet, uint32_t binding);

        std::shared_ptr<IImage> _image;
        std::shared_ptr<Sampler> _sampler;
    };

    class SamplerUniformSetter : public UniformSetter {
        friend Window;

    public:
        static constexpr VkDescriptorType DescriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

        explicit SamplerUniformSetter(VkDescriptorSet vDescriptorSet, uint32_t binding,
            std::shared_ptr<Sampler> sampler)
            : _sampler(sampler) {
            Init(vDescriptorSet, binding);
        };
        virtual ~SamplerUniformSetter() {}

    private:

        void Init(VkDescriptorSet vDescriptorSet, uint32_t binding);

        std::shared_ptr<Sampler> _sampler;
    };

    class ImagesUniformSetter : public UniformSetter {
        friend Window;

    public:
        static constexpr VkDescriptorType DescriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

        explicit ImagesUniformSetter(VkDescriptorSet vDescriptorSet, uint32_t binding,
            const std::vector<std::shared_ptr<IImage>>& images)
        : _images(images) {
            Init(vDescriptorSet, binding);
        };
        virtual ~ImagesUniformSetter() {}

    private:

        void Init(VkDescriptorSet vDescriptorSet, uint32_t binding);

        std::vector<std::shared_ptr<IImage>> _images;
    };

    struct DescriptorPoolData {
        int count = 0;
        VkDescriptorPool vDescriptorPool;
    };

    class Uniform : public RiseObject {
        friend class Window;
        friend class Renderer;

    public:
        explicit Uniform(Core* core, DescriptorPoolData& poolData, const Renderer::SetLayout& layout)
            : RiseObject(core) {
            Init(poolData, layout);
        };

        void Init(DescriptorPoolData& poolData, const Renderer::SetLayout& layout);

        bool Ready() {
            for (auto& setter : setters) {
                if (!setter.second->Ready()) {
                    return false;
                }
            }
            return true;
        }

        VkDescriptorSet vDescriptorSet;
        std::unordered_map<uint16_t, std::shared_ptr<UniformSetter>> setters;
    };
}

#endif /* uniform_h */
