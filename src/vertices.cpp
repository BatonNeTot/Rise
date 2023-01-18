//☀Rise☀
#include "vertices.h"

#include "gpu_allocator.h"
#include "rise.h"

namespace Rise {

    void Vertices::Meta::Setup(const Data& data) {
        std::vector<VerticesType> types;

        for (auto typeStr : data["format"]) {
            types.emplace_back(VerticesType::FromString(typeStr.as<std::string>().c_str()));
        }

        _format = {types};
        _count = data["count"];
    }
    void CustomVertices::Meta::Setup(const Data& data) {
        std::vector<VerticesType> types;

        for (auto typeStr : data["format"]) {
            types.emplace_back(VerticesType::FromString(typeStr.as<std::string>().c_str()));
        }

        _format = { types };
        _count = data["count"];
    }

    void CustomVertices::SetupMeta(const CustomVertices::Meta& meta) {
        _meta = meta;
    }
    void Vertices::LoadFromFile(const std::string& filename) {
        Data data;
        
        {
            std::ifstream i(filename);
            i >> data;
        }

        VkDeviceSize bufferSize = _meta->sizeOf();

        _vertexBuffer = GpuAllocator::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        auto stagingBuffer = GpuAllocator::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        {
            auto memoryPtr = stagingBuffer.MapMemory();
            uint32_t typeIndex = 0;
            uint32_t counter = 0;
            for (auto vec : data) {
                switch (_meta->format().Type(typeIndex)) {
                case VerticesType::Vec2: {
                    _meta->format().InsertValueAt(memoryPtr, counter, typeIndex, glm::vec2(vec["x"], vec["y"]));
                    break;
                }
                case VerticesType::Vec3: {
                    _meta->format().InsertValueAt(memoryPtr, counter, typeIndex, glm::vec3(vec["x"], vec["y"], vec["z"]));
                    break;
                }
                }

                ++typeIndex;
                if (_meta->format().Count() == typeIndex) {
                    typeIndex = 0;
                    ++counter;
                }
            }
        }

        GpuAllocator::CopyBuffer(stagingBuffer, _vertexBuffer, bufferSize);
        GpuAllocator::DestroyBuffer(stagingBuffer);

        MarkLoaded();
    }

    void CustomVertices::Load(const std::vector<uint8_t>& data) {
        VkDeviceSize bufferSize = data.size();

        _vertexBuffer = GpuAllocator::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        auto stagingBuffer = GpuAllocator::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        {
            auto memoryPtr = stagingBuffer.MapMemory();
            memcpy(memoryPtr, data.data(), data.size());
        }
        
        GpuAllocator::CopyBuffer(stagingBuffer, _vertexBuffer, bufferSize);
        GpuAllocator::DestroyBuffer(stagingBuffer);

        MarkLoaded();
    }

    void Vertices::Unload() {
        GpuAllocator::DestroyBuffer(_vertexBuffer);

        MarkUnloaded();
    }
    void CustomVertices::Unload() {
        GpuAllocator::DestroyBuffer(_vertexBuffer);

        MarkUnloaded();
    }
}
