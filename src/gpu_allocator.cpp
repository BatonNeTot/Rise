//☀Rise☀
#include "Rise/gpu_allocator.h"

#include "Rise/rise.h"
#include "Rise/logger.h"

namespace Rise {

    std::unordered_map<uint32_t, GpuAllocator::MemoryData> GpuAllocator::_memoryByType;

    std::unordered_map<VkBuffer, GpuAllocator::AllocationData> GpuAllocator::_bufferAllocations;
    std::unordered_map<VkImage, GpuAllocator::AllocationData> GpuAllocator::_imageAllocations;

    std::shared_mutex GpuAllocator::_allocationLock;

    GpuAllocator::Buffer::MapPtr GpuAllocator::Buffer::MapMemory() const {
        std::unique_lock ul(_allocationLock);
        auto& allocation = _bufferAllocations[_vBuffer];
        auto& memoryByType = _memoryByType.find(allocation.memoryType)->second;
        return memoryByType._memoryBlocks[allocation.blockIndex].MapMemory(allocation.offset);
    }

    GpuAllocator::Buffer::MapPtr::~MapPtr() {
        std::lock_guard<std::recursive_mutex> lg(Rise::Instance()->_deviceLock);
    }

    GpuAllocator::Buffer::MapPtr GpuAllocator::MemoryData::MemoryBlock::MapMemory(ptrdiff_t offset) {
        auto mapPtr = _mapPtr.lock();
        if (!mapPtr) {
            void* pData;
            {
                std::lock_guard<std::recursive_mutex> lg(Rise::Instance()->_deviceLock);
                vkMapMemory(Instance()->_vDevice, _vMemory, 0, _size, 0, &pData);
            }

            mapPtr = { pData, [vMemory = this->_vMemory](void*) {
                vkUnmapMemory(Instance()->_vDevice, vMemory);
            } };
            _mapPtr = mapPtr;
        }
        return { mapPtr, offset };
    }

    // TODO change vulkan flags to "Gpu/Gpu-Cpu/Cpu" enum and support using more fast one, if requested is inaccessible
    GpuAllocator::Buffer GpuAllocator::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkMemoryPropertyFlags ignoreProperties) {
        VkBuffer buffer;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        {
            std::lock_guard<std::recursive_mutex> lg(Instance()->_deviceLock);
            if (vkCreateBuffer(Instance()->_vDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
                Instance()->Logger().Error("failed to create buffer!");
            }
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(Instance()->_vDevice, buffer, &memRequirements);

        VkDeviceMemory memory;
        std::unique_lock ul(_allocationLock);
        auto& allocationData = _bufferAllocations.try_emplace(buffer).first->second;
        Allocate(memRequirements, properties, ignoreProperties, memory, allocationData);

        {
            std::lock_guard<std::recursive_mutex> lg(Instance()->_deviceLock);
            vkBindBufferMemory(Instance()->_vDevice, buffer, memory, allocationData.offset);
        }

        return Buffer(buffer);
    }

    void GpuAllocator::DestroyBuffer(Buffer buffer) {
        std::unique_lock ul(_allocationLock);
        {
            std::lock_guard<std::recursive_mutex> lg(Instance()->_deviceLock);
            vkDestroyBuffer(Instance()->_vDevice, buffer._vBuffer, nullptr);
        }

        auto allocationIt = _bufferAllocations.find(buffer._vBuffer);
        auto& allocation = allocationIt->second;
        auto& memory = _memoryByType.find(allocation.memoryType)->second;
        auto& block = memory._memoryBlocks[allocation.blockIndex];
        memory._logicalBlocks.emplace(allocation.blockIndex, allocation.offset, allocation.size);
        block._blocks.emplace(allocation.offset, allocation.size);

        {
            auto blockIt = block._blocks.begin();
            auto nextIt = blockIt;
            ++nextIt;
            while (nextIt != block._blocks.end()) {
                if (blockIt->_offset + blockIt->_size == nextIt->_offset) {
                    memory._logicalBlocks.erase(
                        MemoryData::LogicalBlock(allocation.blockIndex, nextIt->_offset, nextIt->_size));
                    memory._logicalBlocks.erase(
                        MemoryData::LogicalBlock(allocation.blockIndex, blockIt->_offset, blockIt->_size));

                    blockIt->_size += nextIt->_size;
                    memory._logicalBlocks.emplace(allocation.blockIndex, blockIt->_offset, blockIt->_size);
                    nextIt = block._blocks.erase(nextIt);
                }
                else {
                    blockIt = nextIt;
                    ++nextIt;
                }
            }
        }

        _bufferAllocations.erase(allocationIt);
    }

    void GpuAllocator::CopyBuffer(Buffer srcBuffer, Buffer dstBuffer, VkDeviceSize size) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = Instance()->_vInstantCommandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        std::lock_guard<std::recursive_mutex> lg(Instance()->_deviceLock);
        vkAllocateCommandBuffers(Instance()->_vDevice, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer._vBuffer, dstBuffer._vBuffer, 1, &copyRegion);

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(Instance()->_vGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(Instance()->_vGraphicsQueue);

        vkFreeCommandBuffers(Instance()->_vDevice, Instance()->_vInstantCommandPool, 1, &commandBuffer);
    }

    GpuAllocator::Image GpuAllocator::CreateImage(VkExtent3D& extent, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkMemoryPropertyFlags ignoreProperties /* = 0*/) {
        VkImage image;

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = extent;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags = 0; // Optional

        if (vkCreateImage(Instance()->_vDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            Instance()->Logger().Error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(Instance()->_vDevice, image, &memRequirements);

        VkDeviceMemory memory;
        std::unique_lock ul(_allocationLock);
        auto& allocationData = _imageAllocations.try_emplace(image).first->second;
        Allocate(memRequirements, properties, ignoreProperties, memory, allocationData);

        {
            std::lock_guard<std::recursive_mutex> lg(Instance()->_deviceLock);
            vkBindImageMemory(Instance()->_vDevice, image, memory, allocationData.offset);
        }

        return Image(image);
    }
    void GpuAllocator::DestroyImage(GpuAllocator::Image image) {
        std::unique_lock ul(_allocationLock);
        {
            std::lock_guard<std::recursive_mutex> lg(Instance()->_deviceLock);
            vkDestroyImage(Instance()->_vDevice, image._vImage, nullptr);
        }

        auto allocationIt = _imageAllocations.find(image._vImage);
        auto& allocation = allocationIt->second;
        auto& memory = _memoryByType.find(allocation.memoryType)->second;
        auto& block = memory._memoryBlocks[allocation.blockIndex];
        memory._logicalBlocks.emplace(allocation.blockIndex, allocation.offset, allocation.size);
        block._blocks.emplace(allocation.offset, allocation.size);

        {
            auto blockIt = block._blocks.begin();
            auto nextIt = blockIt;
            ++nextIt;
            while (nextIt != block._blocks.end()) {
                if (blockIt->_offset + blockIt->_size == nextIt->_offset) {
                    memory._logicalBlocks.erase(
                        MemoryData::LogicalBlock(allocation.blockIndex, nextIt->_offset, nextIt->_size));
                    memory._logicalBlocks.erase(
                        MemoryData::LogicalBlock(allocation.blockIndex, blockIt->_offset, blockIt->_size));

                    blockIt->_size += nextIt->_size;
                    memory._logicalBlocks.emplace(allocation.blockIndex, blockIt->_offset, blockIt->_size);
                    nextIt = block._blocks.erase(nextIt);
                }
                else {
                    blockIt = nextIt;
                    ++nextIt;
                }
            }
        }

        _imageAllocations.erase(allocationIt);
    }

    void GpuAllocator::CopyBufferToImage(Buffer srcBuffer, VkDeviceSize size, Image dstImage, VkExtent3D& extent) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = Instance()->_vInstantCommandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        std::lock_guard<std::recursive_mutex> lg(Instance()->_deviceLock);
        vkAllocateCommandBuffers(Instance()->_vDevice, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        ///////////////////////////////////////

        VkImageSubresourceRange range;
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 1;

        VkImageMemoryBarrier imageBarrier_toTransfer = {};
        imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

        imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier_toTransfer.image = dstImage._vImage;
        imageBarrier_toTransfer.subresourceRange = range;

        imageBarrier_toTransfer.srcAccessMask = 0;
        imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        //barrier the image into the transfer-receive layout
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);

        ///////////////////////////////////////

        VkBufferImageCopy copyRegion = {};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;

        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageOffset = {};
        copyRegion.imageExtent = extent;

        //copy the buffer into the image
        vkCmdCopyBufferToImage(commandBuffer, srcBuffer._vBuffer, dstImage._vImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        ///////////////////////////////////////

        VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;

        imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageBarrier_toReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        //barrier the image into the shader readable layout
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);

        ///////////////////////////////////////

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(Instance()->_vGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(Instance()->_vGraphicsQueue);

        vkFreeCommandBuffers(Instance()->_vDevice, Instance()->_vInstantCommandPool, 1, &commandBuffer);
    }

    bool GpuAllocator::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkMemoryPropertyFlags ignoreProperties, uint32_t& memoryType) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(Instance()->_vPhysicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties
                && (memProperties.memoryTypes[i].propertyFlags & ignoreProperties) == 0) {
                memoryType = i;
                return true;
            }
        }

        return false;
    }

    void GpuAllocator::Destroy() {
        for (auto& memoryByType : _memoryByType) {
            for (auto& memoryBlock : memoryByType.second._memoryBlocks) {
                vkFreeMemory(Instance()->_vDevice, memoryBlock._vMemory, nullptr);
            }
        }
    }

    bool GpuAllocator::Allocate(const VkMemoryRequirements& memRequirements, VkMemoryPropertyFlags properties, VkMemoryPropertyFlags ignoreProperties, VkDeviceMemory& memory, AllocationData& data) {
        uint32_t memoryType = 0;
        if (!FindMemoryType(memRequirements.memoryTypeBits, properties, ignoreProperties, memoryType)) {
            Instance()->Logger().Error("failed to find suitable memory type!");
        }

        auto& memoryData = _memoryByType.try_emplace(memoryType, memoryType).first->second;
        auto result = memoryData.Allocate(memRequirements, data);

        if (result) {
            memory = memoryData._memoryBlocks[data.blockIndex]._vMemory;
        }
        return result;
    }

    bool GpuAllocator::MemoryData::Allocate(const VkMemoryRequirements& memRequirements, AllocationData& data) {
        if (memRequirements.size > RISE_GPU_ALLOCATOR_BLOCK_SIZE) {
            Instance()->Logger().Error("block size less then requested memory size!");
        }

        // getting smallest block
        auto it = _logicalBlocks.lower_bound({ memRequirements.size });

        // getting alignment into account
        auto alignMask = memRequirements.alignment - 1;
        VkDeviceSize offset = 0;

        while (it != _logicalBlocks.end()) {
            offset = (it->_offset + alignMask) & ~alignMask;

            // looking for smallest block into which alignment fit
            if (memRequirements.size <= it->_size - (offset - it->_offset)) {
                break;
            }

            ++it;
        }

        // if there is not, create new block
        // freshly created blocks always please alignment
        if (it == _logicalBlocks.end()) {
            it = CreateMemoryBlock(_memoryType);
            offset = it->_offset;
        }

        auto& memoryBlock = _memoryBlocks[it->_blockIndex];
        memoryBlock._blocks.erase(memoryBlock._blocks.find({ it->_offset }));

        data.memoryType = _memoryType;
        data.blockIndex = it->_blockIndex;
        data.offset = offset;
        data.size = memRequirements.size;

        auto localOffset = offset - it->_offset;

        // adding offset block
        if (localOffset > 0) {
            auto newOffset = it->_offset;
            auto newSize = localOffset;
            memoryBlock._blocks.emplace(newOffset, newSize);

            _logicalBlocks.emplace(data.blockIndex, newOffset, newSize);
        }

        // adding tail block
        if (it->_size - localOffset > memRequirements.size) {
            auto newOffset = it->_offset + localOffset + data.size;
            auto newSize = it->_size - localOffset - data.size;
            memoryBlock._blocks.emplace(newOffset, newSize);

            _logicalBlocks.emplace(data.blockIndex, newOffset, newSize);
        }

        // deleting what we created
        _logicalBlocks.erase(it);

        return true;
    }

    decltype(std::multiset<GpuAllocator::MemoryData::LogicalBlock>{}.begin())
        GpuAllocator::MemoryData::CreateMemoryBlock(uint32_t memoryType) {

        uint32_t blockIndex = static_cast<uint32_t>(_memoryBlocks.size());
        auto& memoryBlock = _memoryBlocks.emplace_back(RISE_GPU_ALLOCATOR_BLOCK_SIZE);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memoryBlock._size;
        allocInfo.memoryTypeIndex = memoryType;

        if (vkAllocateMemory(Instance()->_vDevice, &allocInfo, nullptr, &memoryBlock._vMemory) != VK_SUCCESS) {
            Instance()->Logger().Error("failed to allocate buffer memory!");
        }

        memoryBlock._blocks.emplace(0, memoryBlock._size);
        return _logicalBlocks.emplace(blockIndex, 0, memoryBlock._size);
    }

    // TODO change vulkan flags to "Gpu/Gpu-Cpu/Cpu" enum and support using more fast one, if requested is inaccessible
    GpuStackAllocator::Buffer GpuStackAllocator::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkMemoryPropertyFlags ignoreProperties) {
        VkBuffer buffer;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        std::lock_guard<std::recursive_mutex> lg(Instance()->_deviceLock);
        if (vkCreateBuffer(Instance()->_vDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            Instance()->Logger().Error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(Instance()->_vDevice, buffer, &memRequirements);

        VkDeviceMemory memory;
        AllocationData allocationData;
        Allocate(memRequirements, properties, ignoreProperties, memory, allocationData);

        vkBindBufferMemory(Instance()->_vDevice, buffer, memory, allocationData.offset);

        return Buffer(buffer);
    }

    bool GpuStackAllocator::Allocate(const VkMemoryRequirements& memRequirements, VkMemoryPropertyFlags properties, VkMemoryPropertyFlags ignoreProperties, VkDeviceMemory& memory, AllocationData& data) {
        uint32_t memoryType = 0;
        if (!GpuAllocator::FindMemoryType(memRequirements.memoryTypeBits, properties, ignoreProperties, memoryType)) {
            Instance()->Logger().Error("failed to find suitable memory type!");
        }

        auto& memoryData = _memoryByType.try_emplace(memoryType).first->second;
        auto result = memoryData.Allocate(memoryType, memRequirements, data);

        return result;
    }

    bool GpuStackAllocator::MemoryData::Allocate(uint32_t memoryType, const VkMemoryRequirements& memRequirements, AllocationData& data) {
        // TODO take into account alignment
        if (memRequirements.size > RISE_GPU_ALLOCATOR_BLOCK_SIZE) {
            Instance()->Logger().Error("block size less then requested memory size!");
        }

        if (_memoryBlocks.empty() || _memoryBlocks.back().freeSpace() < memRequirements.size) {
            CreateMemoryBlock(memoryType);
        }

        auto& memoryBlock = _memoryBlocks.back();

        data.offset = memoryBlock._usedSize;

        memoryBlock._usedSize += memRequirements.size;

        return true;
    }
    
    void GpuStackAllocator::MemoryData::CreateMemoryBlock(uint32_t memoryType) {

        uint32_t blockIndex = static_cast<uint32_t>(_memoryBlocks.size());
        auto& memoryBlock = _memoryBlocks.emplace_back(RISE_GPU_ALLOCATOR_BLOCK_SIZE);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memoryBlock._size;
        allocInfo.memoryTypeIndex = memoryType;

        if (vkAllocateMemory(Instance()->_vDevice, &allocInfo, nullptr, &memoryBlock._vMemory) != VK_SUCCESS) {
            Instance()->Logger().Error("failed to allocate buffer memory!");
        }
    }

}
