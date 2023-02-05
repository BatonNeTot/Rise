//☀Rise☀
#include "Rise/image.h"

#include "Rise/gpu_allocator.h"

#include "stb_image.h"

#include <iostream>

namespace Rise {

	Sampler::Sampler(Core* core)
		: RiseObject(core) {}
	Sampler::~Sampler() {
		vkDestroySampler(Instance()->_vDevice, _vSampler, nullptr);
	}

	void Sampler::Load(const CombinedValue<Sampler::Attachment>& value) {
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_NEAREST;
		samplerInfo.minFilter = VK_FILTER_NEAREST;

		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(Instance()->_vPhysicalDevice, &properties);

		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;

		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		if (vkCreateSampler(Instance()->_vDevice, &samplerInfo, nullptr, &_vSampler) != VK_SUCCESS) {
			Error("failed to create texture sampler!");
		}
	}

	void Image::Meta::Setup(const Data& data) {
		vFormat = VK_FORMAT_R8G8B8A8_SRGB;
		size.width = data["width"];
		size.height = data["height"];
	}

	void IImage::Load(VkImage vImage, VkImageAspectFlags aspectFlags, bool singleByte /*= false*/) {
		_vImage = vImage;

		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = _vImage;

		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = GetMeta()->vFormat;

		if (!singleByte) {
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		}
		else {
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_R;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_R;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_R;
		}

		createInfo.subresourceRange.aspectMask = aspectFlags;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		std::lock_guard<std::recursive_mutex> lg(Instance()->_deviceLock);
		if (vkCreateImageView(Instance()->_vDevice, &createInfo, nullptr, &_vImageView) != VK_SUCCESS) {
			Error("failed to create image views!");
		}

		MarkLoaded();
	}

	void IImage::Unload() {
		vkDestroyImageView(Instance()->_vDevice, _vImageView, nullptr);
		MarkUnloaded();
	}

	Framebuffer& IImage::getOrCreateFramebuffer(const RenderPass& renderPass) {
		return _framebuffers.try_emplace(&renderPass, Instance(), renderPass, *this).first->second;
	}

	void Image::LoadFromFile(const std::string& filename) {
		int texWidth, texHeight, texChannels;

		stbi_uc* pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

		if (!pixels) {
			Error("Failed to load texture file " + filename);
			return;
		}

		if (texWidth != GetMeta()->size.width ||
			texHeight != GetMeta()->size.height) {
			Error("Meta size doesn't equal actual texture size" + filename);
			return;
		}

		void* pixel_ptr = pixels;
		VkDeviceSize imageSize = GetMeta()->size.width * GetMeta()->size.height * 4;

		//allocate temporary buffer for holding texture data to upload
		auto stagingBuffer = GpuAllocator::CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		{
			//copy data to buffer
			auto data = stagingBuffer.MapMemory();

			memcpy(data, pixel_ptr, static_cast<size_t>(imageSize));
		}
		//we no longer need the loaded data, so we can free the pixels as they are now in the staging buffer
		stbi_image_free(pixels);

		////////////////////////////////////////////////////////////////////////////////////////////////

		VkExtent3D imageExtent;
		imageExtent.width = static_cast<uint32_t>(texWidth);
		imageExtent.height = static_cast<uint32_t>(texHeight);
		imageExtent.depth = 1;
		
		_image = GpuAllocator::CreateImage(imageExtent, GetMeta()->vFormat, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

		GpuAllocator::CopyBufferToImage(stagingBuffer, imageSize, _image, imageExtent);
		GpuAllocator::DestroyBuffer(stagingBuffer);

		Load(_image.vImage(), VK_IMAGE_ASPECT_COLOR_BIT);
	}

	void Image::Unload() {
		IImage::Unload();
		GpuAllocator::DestroyImage(_image);
	}

	void IImage::SetupMeta(const IImage::Meta& meta) {
		_meta = &meta;
	}

	void TargetImage::Load() {

		VkExtent3D imageExtent;
		imageExtent.width = static_cast<uint32_t>(GetMeta()->size.width);
		imageExtent.height = static_cast<uint32_t>(GetMeta()->size.height);
		imageExtent.depth = 1;

		_image = GpuAllocator::CreateImage(imageExtent, GetMeta()->vFormat, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

		IImage::Load(_image.vImage(), VK_IMAGE_ASPECT_COLOR_BIT);
	}
	void TargetImage::Unload() {
		IImage::Unload();
		GpuAllocator::DestroyImage(_image);
	}

	GpuAllocator::Buffer::MapPtr CustomImage::LoadPrepare(uint32_t pixelSize) {
		VkDeviceSize imageSize = GetMeta()->size.width * GetMeta()->size.height * pixelSize;

		//allocate temporary buffer for holding texture data to upload
		_stagingBuffer = GpuAllocator::CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		return _stagingBuffer.MapMemory();

	}
	void CustomImage::LoadCommit(uint32_t pixelSize) {
		VkDeviceSize imageSize = GetMeta()->size.width * GetMeta()->size.height * pixelSize;

		VkExtent3D imageExtent;
		imageExtent.width = static_cast<uint32_t>(GetMeta()->size.width);
		imageExtent.height = static_cast<uint32_t>(GetMeta()->size.height);
		imageExtent.depth = 1;

		_image = GpuAllocator::CreateImage(imageExtent, GetMeta()->vFormat, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

		GpuAllocator::CopyBufferToImage(_stagingBuffer, imageSize, _image, imageExtent);
		GpuAllocator::DestroyBuffer(_stagingBuffer);

		IImage::Load(_image.vImage(), VK_IMAGE_ASPECT_COLOR_BIT, true);
	}

	void CustomImage::Unload() {
		IImage::Unload();
		GpuAllocator::DestroyImage(_image);
	}

	void N9Slice::Load() {
		_image = Instance()->ResourceGenerator().Get<Image>(meta()->textureId);
	}
}
