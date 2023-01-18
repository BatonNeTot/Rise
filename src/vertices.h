//☀Rise☀
#ifndef vertices_h
#define vertices_h

#include "rise_object.h"
#include "resource.h"
#include "gpu_allocator.h"
#include "utils.h"

#include <vulkan/vulkan.h>
#include "pugixml.hpp"

#include <tuple>
#include <utility>
#include <array>
#include <vector>
#include <fstream>

namespace Rise {

    class VerticesType : public BaseType {
    public:
        VerticesType() = default;
        constexpr VerticesType(Enum value) : BaseType(value) {}
        constexpr VerticesType(BaseType value) : BaseType(value) {}

        constexpr static VerticesType FromString(const char* str, VerticesType def = {}) {
            return BaseType::FromString(str, def);
        }

        constexpr VkFormat Format() const {
            switch (*this) {
            case Vec2: return VK_FORMAT_R32G32_SFLOAT;
            case Vec3: return VK_FORMAT_R32G32B32_SFLOAT;
            default: return VK_FORMAT_UNDEFINED;
            }
        }
    };

    class VerticesFormat : public Metadata<VerticesType> {
    public:

        VerticesFormat() = default;
        VerticesFormat(const std::initializer_list<VerticesType>& initializer)
            : VerticesFormat(std::vector<VerticesType>( initializer )) {}
        VerticesFormat(const std::vector<VerticesType>& types) 
            : Metadata<VerticesType>(types){};

        void FillBindingDescription(std::vector<VkVertexInputBindingDescription>& container) const {
            auto& bindingDescription = container.emplace_back();

            bindingDescription.binding = 0;
            bindingDescription.stride = SizeOf();
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        }

        void FillAttributeDescriptions(std::vector<VkVertexInputAttributeDescription>& container) const {
            for (uint32_t i = 0; i < Count(); ++i) {
                auto& attributeDescription = container.emplace_back();

                attributeDescription.binding = 0;
                attributeDescription.location = i;
                attributeDescription.format = Type(i).Format();
                attributeDescription.offset = Offset(i);
            }
        }
    };

    class IVertices : public ResourceBase, public RiseObject {
    public:

        explicit IVertices(Core* core)
            : RiseObject(core) {}

        class IMeta {
        public:
            virtual const VerticesFormat& format() const = 0;
            virtual uint32_t count() const = 0;

            virtual uint32_t sizeOf() const {
                return count() * format().SizeOf();
            }
        };

        virtual const IMeta& meta() const = 0;
        virtual const GpuAllocator::Buffer& buffer() const = 0;
    };

    class Vertices : public IVertices {
    public:

        using Ptr = std::shared_ptr<Vertices>;

        constexpr static const char* Ext = "mesh";

        class Meta : public IVertices::IMeta {
        public:
            Meta() = default;
            Meta(uint32_t count, const VerticesFormat& format)
                : _count(count), _format(format) {}

            void Setup(const Data& data);

            const VerticesFormat& format() const override {
                return _format;
            }
            uint32_t count() const override {
                return _count;
            }

        private:

            VerticesFormat _format;
            uint32_t _count = 0;
        };

        Vertices(Core* core, const Meta* meta)
            : IVertices(core), _meta(meta) {};
        ~Vertices() override {
            if (IsLoaded()) {
                Unload();
            }
        }

        void LoadFromFile(const std::string& filename);
        void Unload() override;

        const Meta& meta() const override {
            return *_meta;
        }

        const GpuAllocator::Buffer& buffer() const override {
            return _vertexBuffer;
        }

    private:

        const Meta* _meta = nullptr;
        GpuAllocator::Buffer _vertexBuffer;
    };

    class CustomVertices : public IVertices {
    public:

        using Ptr = std::shared_ptr<CustomVertices>;

        explicit CustomVertices(Core* core)
            : IVertices(core) {}
        ~CustomVertices() override {
            if (IsLoaded()) {
                Unload();
            }
        }

        constexpr static const char* Ext = "mesh";

        class Meta : public IVertices::IMeta {
        public:
            Meta() = default;
            Meta(uint32_t count, const VerticesFormat& format)
                : _count(count), _format(format) {}

            void Setup(const Data& data);

            const VerticesFormat& format() const override {
                return _format;
            }
            uint32_t count() const override {
                return _count;
            }

        private:

            VerticesFormat _format;
            uint32_t _count = 0;
        };

        void SetupMeta(const Meta& meta);
        void Load(const std::vector<uint8_t>& data);
        void Unload() override;

        const Meta& meta() const override {
            return _meta;
        }

        const GpuAllocator::Buffer& buffer() const override {
            return _vertexBuffer;
        }

    private:

        Meta _meta;
        GpuAllocator::Buffer _vertexBuffer;
    };
}

#endif /* vertices_h */
