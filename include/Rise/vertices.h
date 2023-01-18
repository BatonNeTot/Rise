//☀Rise☀
#ifndef rise_vertices_h
#define rise_vertices_h


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
            case vec2: return VK_FORMAT_R32G32_SFLOAT;
            case vec3: return VK_FORMAT_R32G32B32_SFLOAT;
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
                attributeDescription.format = VerticesType(Type(i)).Format();
                attributeDescription.offset = Offset(i);
            }
        }

    private:
        std::vector<VerticesType> _types;
    };
}

#endif /* rise_vertices_h */
