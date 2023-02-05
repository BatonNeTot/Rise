//☀Rise☀
#include "Rise/uniform.h"

#include "Rise/window.h"
#include "Rise/gpu_allocator.h"
#include "Rise/rise.h"

namespace Rise {

    void Uniform::Init(DescriptorPoolData& poolData, const Renderer::SetLayout& layout) {
        VkDescriptorSetLayout layouts[1] = { layout._vDescriptorSetLayout };

        uint32_t count = 1;

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = poolData.vDescriptorPool;
        allocInfo.descriptorSetCount = count;
        allocInfo.pSetLayouts = layouts;

        std::lock_guard<std::recursive_mutex> lg(Instance()->_deviceLock);
        if (vkAllocateDescriptorSets(Instance()->_vDevice, &allocInfo, &vDescriptorSet) != VK_SUCCESS) {
            Error("failed to allocate descriptor sets!");
        }

        poolData.count += count;
    }

    void BufferUniformSetter::InitBuffer(VkDeviceSize bufferSize) {
        uniformBuffer = GpuAllocator::CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    void BufferUniformSetter::Init(VkDescriptorSet vDescriptorSet, uint32_t binding, VkDeviceSize size) {
        InitBuffer(size);

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffer.vBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = size;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = vDescriptorSet;
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = DescriptorType;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;
        descriptorWrite.pImageInfo = nullptr;
        descriptorWrite.pTexelBufferView = nullptr;

        vkUpdateDescriptorSets(Instance()->_vDevice, 1, &descriptorWrite, 0, nullptr);
    }

    void BufferUniformSetter::UpdateUniformBuffer(const std::vector<uint8_t>& data) {
        auto ptrData = uniformBuffer.MapMemory();
        memcpy(ptrData, data.data(), data.size());
    }

    void BufferUniformSetter::Destroy() {
        GpuAllocator::DestroyBuffer(uniformBuffer);
    }


    void ImageUniformSetter::Init(VkDescriptorSet vDescriptorSet, uint32_t binding) {
        if (!_image->IsLoaded()) {
            return;
        }

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = _image->vImageView();
        imageInfo.sampler = _sampler->vSampler();

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = vDescriptorSet;
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = DescriptorType;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = nullptr;
        descriptorWrite.pImageInfo = &imageInfo;
        descriptorWrite.pTexelBufferView = nullptr;

        vkUpdateDescriptorSets(Instance()->_vDevice, 1, &descriptorWrite, 0, nullptr);
    }

    void SamplerUniformSetter::Init(VkDescriptorSet vDescriptorSet, uint32_t binding) {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = nullptr;
        imageInfo.sampler = _sampler->vSampler();

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = vDescriptorSet;
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = DescriptorType;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = nullptr;
        descriptorWrite.pImageInfo = &imageInfo;
        descriptorWrite.pTexelBufferView = nullptr;

        vkUpdateDescriptorSets(Instance()->_vDevice, 1, &descriptorWrite, 0, nullptr);
    }

    void ImagesUniformSetter::Init(VkDescriptorSet vDescriptorSet, uint32_t binding) {
        std::vector<VkDescriptorImageInfo> imageInfos(_images.size());

        for (auto i = 0; i < _images.size(); ++i) {
            imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfos[i].imageView = _images[i]->vImageView();
            imageInfos[i].sampler = nullptr;
        }

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = vDescriptorSet;
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = DescriptorType;
        descriptorWrite.descriptorCount = static_cast<uint32_t>(_images.size());
        descriptorWrite.pBufferInfo = nullptr;
        descriptorWrite.pImageInfo = imageInfos.data();
        descriptorWrite.pTexelBufferView = nullptr;

        vkUpdateDescriptorSets(Instance()->_vDevice, 1, &descriptorWrite, 0, nullptr);
    }

}
