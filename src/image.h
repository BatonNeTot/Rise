//☀Rise☀
#ifndef image_h
#define image_h

#include "rise_object.h"
#include "resource.h"
#include "framebuffer.h"
#include "gpu_allocator.h"

#include <vulkan/vulkan_core.h>

namespace Rise {

    class Sampler : public ResourceBase, public RiseObject {
    public:

        enum class Attachment {
            Color
        };

        using Ptr = std::shared_ptr<RenderPass>;
        constexpr static bool InstantLoad = true;

        explicit Sampler(Core* core);
        ~Sampler();

        void Load(const CombinedValue<Sampler::Attachment>& value);

        VkSampler vSampler() const {
            return _vSampler;
        }

    private:

        VkSampler _vSampler = nullptr;
    };

	class IImage : public ResourceBase, public RiseObject {
	public:

        using Ptr = std::shared_ptr<IImage>;
        constexpr static const char* Ext = "png";

        explicit IImage(Core* core)
            : RiseObject(core) {}
        virtual ~IImage() {
            if (IsLoaded()) {
                Unload();
            }
        }

        struct Meta {
            VkFormat vFormat;
            VkExtent2D size;
        };

        void SetupMeta(const Meta& meta);

        const Meta* GetMeta() const {
            return _meta;
        }

        VkImage vImage() const {
            return _vImage;
        }

        VkImageView vImageView() const {
            return _vImageView;
        }

        void Load(VkImage vImage, VkImageAspectFlags aspectFlags, bool singleByte = false);

        void Unload() override;

        Framebuffer& getOrCreateFramebuffer(const RenderPass& renderPass);

	private:

        const Meta* _meta = nullptr;
        VkImage _vImage = nullptr;

        VkImageView _vImageView = nullptr;
        std::unordered_map<const RenderPass*, Framebuffer> _framebuffers;

	};

    class Image : public IImage {
    public:

        using Ptr = std::shared_ptr<Image>;

        class Meta : public IImage::Meta {
        public:
            void Setup(const Data& data);

        };

        Image(Core* core, const Meta* meta)
            : IImage(core) {
            SetupMeta(*meta);
        };
        ~Image() override {
            if (IsLoaded()) {
                Unload();
            }
        }

        void LoadFromFile(const std::string& filename);
        void Unload() override;

        GpuAllocator::Image _image;
    };

    class SwapchainImage : public IImage {
    public:

        SwapchainImage()
            : IImage(nullptr) {}
        explicit SwapchainImage(Core* core)
            : IImage(core) {}

        using Ptr = std::shared_ptr<SwapchainImage>;

    };

    class TargetImage : public IImage {
    public:

        using Ptr = std::shared_ptr<TargetImage>;
        TargetImage(Core* core, const Meta* meta)
            : IImage(core) {
            SetupMeta(*meta);
        };
        ~TargetImage() override {
            if (IsLoaded()) {
                Unload();
            }
        }

        void Load();
        void Unload() override;

    private:

        GpuAllocator::Image _image;

    };

    class CustomImage : public IImage {
    public:

        using Ptr = std::shared_ptr<CustomImage>;

        class Meta : public IImage::Meta {
        public:
            void Setup(const Data& data);

        };

        CustomImage(Core* core, const Meta* meta)
            : IImage(core) {
            SetupMeta(*meta);
        };
        ~CustomImage() override {
            if (IsLoaded()) {
                Unload();
            }
        }

        GpuAllocator::Buffer::MapPtr LoadPrepare(uint32_t pixelSize);
        void LoadCommit(uint32_t pixelSize);
        void Unload() override;

        GpuAllocator::Buffer _stagingBuffer;
        GpuAllocator::Image _image;
    };

    class N9Slice : public ResourceBase, public RiseObject {
    public:

        using Ptr = std::shared_ptr<N9Slice>;
        constexpr static const char* Ext = "9slce";

        constexpr static bool MetaOnly = true;

        struct Meta {
            std::string textureId;
            Padding2D metrics;

            void Setup(const Data& data) {
                textureId = data["textureId"].as<std::string>();
                metrics = Padding2D(data);
            }
        };

        N9Slice(Core* core, const Meta* meta)
            : RiseObject(core) {
            SetupMeta(*meta);
        }
        virtual ~N9Slice() {
            if (IsLoaded()) {
                Unload();
            }
        }

        void SetupMeta(const Meta& meta) {
            _meta = &meta;
        }

        const Meta* meta() const {
            return _meta;
        }

        std::shared_ptr<IImage> texture() const {
            return _image;
        }

        void Load();

        void Unload() override {
            _image.reset();
            MarkUnloaded();
        }

    private:

        const Meta* _meta = nullptr;
        std::shared_ptr<IImage> _image;
    };

}

#endif /* image_h */
