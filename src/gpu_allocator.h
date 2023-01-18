//☀Rise☀
#ifndef gpu_allocator_h
#define gpu_allocator_h

#include "utils.h"

#include <vulkan/vulkan.h>

#include <unordered_map>
#include <set>
#include <vector>
#include <shared_mutex>

#ifndef RISE_GPU_ALLOCATOR_BLOCK_SIZE
#define RISE_GPU_ALLOCATOR_BLOCK_SIZE (1024 * 1024 * 16) 
#endif //RISE_GPU_ALLOCATOR_BLOCK_SIZE

namespace Rise {

    class GpuAllocator {
    public:

        struct Buffer {
            Buffer() = default;

            struct MapPtr {
            public:
                MapPtr(std::shared_ptr<void> mapPtr, ptrdiff_t offset)
                    : _offset(offset), _mapPtr(mapPtr) {}

                ~MapPtr();

                operator void* () const {
                    return ptr();
                }

                void* ptr() const {
                    return reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(_mapPtr.get()) + _offset);
                }

            private:

                ptrdiff_t _offset;
                std::shared_ptr<void> _mapPtr;
            };

            MapPtr MapMemory() const;

            VkBuffer vBuffer() const {
                return _vBuffer;
            }

        private:
            friend GpuAllocator;
            Buffer(VkBuffer vBuffer) : _vBuffer(vBuffer) {}

            VkBuffer _vBuffer = nullptr;
        };

        struct Image {
            Image() = default;

            VkImage vImage() const {
                return _vImage;
            }

        private:
            friend GpuAllocator;
            Image(VkImage vImage) : _vImage(vImage) {}

            VkImage _vImage = nullptr;
        };

        static Buffer CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkMemoryPropertyFlags ignoreProperties = 0);
        static void DestroyBuffer(Buffer buffer);

        static void CopyBuffer(Buffer srcBuffer, Buffer dstBuffer, VkDeviceSize size);

        static Image CreateImage(VkExtent3D& extent, VkFormat format, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkMemoryPropertyFlags ignoreProperties = 0);
        static void DestroyImage(Image image);

        static void CopyBufferToImage(Buffer srcBuffer, VkDeviceSize size, Image dstImage, VkExtent3D& extent);

        static void Destroy();

    private:

        friend class GpuStackAllocator;
        struct AllocationData {
            uint32_t memoryType = std::numeric_limits<uint32_t>::max();
            uint32_t blockIndex = std::numeric_limits<uint32_t>::max();
            VkDeviceSize offset = 0;
            VkDeviceSize size = 0;
        };

        static bool Allocate(const VkMemoryRequirements& memRequirements, VkMemoryPropertyFlags properties, VkMemoryPropertyFlags ignoreProperties, VkDeviceMemory& memory, AllocationData& data);

        static bool FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkMemoryPropertyFlags ignoreProperties, uint32_t& memoryType);

        struct MemoryData {

            struct LogicalBlock {

                LogicalBlock(VkDeviceSize size) : _size(size) {}
                LogicalBlock(uint32_t blockIndex, VkDeviceSize offset, VkDeviceSize size) 
                    : _blockIndex(blockIndex), _offset(offset), _size(size) {}

                friend bool operator<(const LogicalBlock& lhs, const LogicalBlock& rhs) {
                    return lhs._blockIndex == std::numeric_limits<uint32_t>::max()
                        || rhs._blockIndex == std::numeric_limits<uint32_t>::max()
                        ? lhs._size < rhs._size : lhs._offset < rhs._offset;
                }

                uint32_t _blockIndex = std::numeric_limits<uint32_t>::max();
                VkDeviceSize _offset = 0;
                VkDeviceSize _size = 0;

            };

            struct MemoryBlock {

                explicit MemoryBlock(VkDeviceSize size) : _size(size) {}

                VkDeviceMemory _vMemory;
                VkDeviceSize _size = 0;

                std::weak_ptr<void> _mapPtr;

                Buffer::MapPtr MapMemory(ptrdiff_t offset);

                struct Block {

                    Block(VkDeviceSize offset)
                        : _offset(offset) {}
                    Block(VkDeviceSize offset, VkDeviceSize size)
                        : _offset(offset), _size(size) {}

                    friend bool operator<(const Block& lhs, const Block& rhs) {
                        return lhs._offset < rhs._offset;
                    }

                    VkDeviceSize _offset = 0;
                    mutable VkDeviceSize _size = 0;
                };

                std::set<Block> _blocks;
            };

            MemoryData(uint32_t memoryType) : _memoryType(memoryType) {}

            bool Allocate(const VkMemoryRequirements& memRequirements, AllocationData& data);
            decltype(std::multiset<LogicalBlock>{}.begin()) CreateMemoryBlock(uint32_t memoryType);

            uint32_t _memoryType;

            std::multiset<LogicalBlock> _logicalBlocks;

            std::vector<MemoryBlock> _memoryBlocks;
        };

        static std::unordered_map<uint32_t, MemoryData> _memoryByType;

        static std::unordered_map<VkBuffer, AllocationData> _bufferAllocations;
        static std::unordered_map<VkImage, AllocationData> _imageAllocations;

        static std::shared_mutex _allocationLock;
    };

    class GpuStackAllocator {
    public:

        class Buffer {
            Buffer() = default;

            void MapMemory(void** ppData) const;
            void UnmapMemory() const;

            VkBuffer vBuffer() const {
                return _vBuffer;
            }

        private:
            friend GpuStackAllocator;
            Buffer(VkBuffer vBuffer) : _vBuffer(vBuffer) {}

            VkBuffer _vBuffer = nullptr;
        };

        Buffer CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkMemoryPropertyFlags ignoreProperties = 0);

        void Reset();

    private:

        struct AllocationData {
            VkDeviceSize offset = 0;
        };

        bool Allocate(const VkMemoryRequirements& memRequirements, VkMemoryPropertyFlags properties, VkMemoryPropertyFlags ignoreProperties, VkDeviceMemory& memory, AllocationData& data);

        struct MemoryData {

            struct MemoryBlock {
                explicit MemoryBlock(VkDeviceSize size) : _size(size) {}

                VkDeviceSize freeSpace() const {
                    return _size - _usedSize;
                }

                VkDeviceMemory _vMemory;
                VkDeviceSize _size = 0;
                VkDeviceSize _usedSize = 0;
            };

            MemoryData() = default;

            bool Allocate(uint32_t memoryType, const VkMemoryRequirements& memRequirements, AllocationData& data);
            void CreateMemoryBlock(uint32_t memoryType);

            std::vector<MemoryBlock> _memoryBlocks;
        };

        std::unordered_map<uint32_t, MemoryData> _memoryByType;

    };

}

#endif /* gpu_allocator_h */
