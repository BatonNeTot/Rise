//☀Rise☀
#ifndef render_target_h
#define render_target_h

#include "resource.h"
#include "image.h"
#include "uniform.h"
#include "draw.h"
#include "shader.h"
#include "node/node.h"

#include <vulkan/vulkan.h>

#include <vector>
#include <unordered_map>

namespace Rise {

#ifndef DESCRIPTOR_POOL_SIZE
#define DESCRIPTOR_POOL_SIZE 10
#endif

    class IRenderTarget : public NodeParent, public ResourceBase, public RiseObject {
    public:

        friend Draw::Context;

        explicit IRenderTarget(Core* core)
            : RiseObject(core) {}
        virtual ~IRenderTarget() {
            for (auto& imageData : _imageDatas) {
                imageData.Unload();
            }
        }

        virtual IImage* CurrentImage() = 0;

        virtual const IImage::Meta* imageMeta() const = 0;

        VkSemaphore Draw() {
            if (_currentGlobalFrame == Instance()->_globalFrameCounter) {
                return _imageDatas[_imageDataIndex].vRenderFinishedSemaphore;
            }
            _currentGlobalFrame = Instance()->_globalFrameCounter;

            auto& imageData = PrepareImageData();

            RecordCommandBuffer(imageData.vCommandBuffer, CurrentImage());

            ResetDrawResources();

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            std::vector<VkSemaphore> waitSemaphores;

            VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
            submitInfo.pWaitSemaphores = waitSemaphores.data();
            submitInfo.pWaitDstStageMask = waitStages;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &imageData.vCommandBuffer;

            VkSemaphore signalSemaphores[] = { imageData.vRenderFinishedSemaphore };
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = signalSemaphores;

            {
                std::lock_guard<std::recursive_mutex> lg(Instance()->_deviceLock);
                auto result = vkQueueSubmit(Instance()->_vGraphicsQueue, 1, &submitInfo, imageData.vInFlightFence);
                if (result != VK_SUCCESS) {
                    Error("failed to submit draw command buffer!");
                }
            }

            return imageData.vRenderFinishedSemaphore;
        }

    private:

        virtual std::vector<VkSemaphore> WaitSemaphores() const {
            return {};
        }

        void RecordCommandBuffer(VkCommandBuffer commandBuffer, IImage* image) {

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = 0; // Optional
            beginInfo.pInheritanceInfo = nullptr; // Optional

            {
                std::lock_guard<std::recursive_mutex> lg(Instance()->_deviceLock);
                if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
                    Error("failed to begin recording command buffer!");
                }
            }

            Draw::Context context(*this, commandBuffer, image);
            _imageDatas[_imageDataIndex].uniforms.emplace_back();

            drawImpl(context);

            if (_renderPass.HasValue() && _renderPass.Value()) {
                _renderPass.Value()->EndRecordCommandBuffer(commandBuffer);
            }
            else {
                // TODO doesn't work
                // for now just be sure that something would be rendered
                VkImageSubresourceRange range;
                range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                range.baseMipLevel = 0;
                range.levelCount = 1;
                range.baseArrayLayer = 0;
                range.layerCount = 1;

                VkImageMemoryBarrier imageBarrier_toTransfer = {};
                imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

                imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                imageBarrier_toTransfer.image = image->vImage();
                imageBarrier_toTransfer.subresourceRange = range;

                imageBarrier_toTransfer.srcAccessMask = 0;
                imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_NONE_KHR;

                vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_NONE_KHR, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);
            }

            {
                std::lock_guard<std::recursive_mutex> lg(Instance()->_deviceLock);
                if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
                    Error("failed to record command buffer!");
                }
            }
        }

        struct SingleImageData : public RiseObject {
            explicit SingleImageData(Core* core)
                : RiseObject(core) {}

            void Load() {
                CreateCommandBuffer();
                SyncObjects();
            }

            void Unload() {
                vkDestroySemaphore(Instance()->_vDevice, vRenderFinishedSemaphore, nullptr);
                vkDestroyFence(Instance()->_vDevice, vInFlightFence, nullptr);

                for (auto& descriptorPoolDataPerLayout : descriptorPoolDatas) {
                    for (auto& descriptorPoolData : descriptorPoolDataPerLayout.second) {
                        vkDestroyDescriptorPool(Instance()->_vDevice, descriptorPoolData.vDescriptorPool, nullptr);
                    }
                }
            }

            void CreateCommandBuffer() {
                VkCommandBufferAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                allocInfo.commandPool = Instance()->_vCommandPool;
                allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                allocInfo.commandBufferCount = 1;

                std::lock_guard<std::recursive_mutex> lg(Instance()->_deviceLock);
                if (vkAllocateCommandBuffers(Instance()->_vDevice, &allocInfo, &vCommandBuffer) != VK_SUCCESS) {
                    Error("failed to allocate command buffers!");
                }
            }

            void SyncObjects() {
                VkSemaphoreCreateInfo semaphoreInfo{};
                semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

                VkFenceCreateInfo fenceInfo{};
                fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

                std::lock_guard<std::recursive_mutex> lg(Instance()->_deviceLock);
                if (vkCreateSemaphore(Instance()->_vDevice, &semaphoreInfo, nullptr, &vRenderFinishedSemaphore) != VK_SUCCESS ||
                    vkCreateFence(Instance()->_vDevice, &fenceInfo, nullptr, &vInFlightFence) != VK_SUCCESS) {

                    Error("failed to create semaphores!");
                }
            }

            using PoolKey = CombinedKey<std::unordered_map<VkDescriptorType, uint32_t>>;

            DescriptorPoolData& CreateDescriptorPoolData(const PoolKey & key) {
                const auto& map = key.Get<0>();

                std::vector<VkDescriptorPoolSize> poolSizes;
                poolSizes.reserve(map.size());

                for (const auto& [type, count] : map) {
                    auto& poolSize = poolSizes.emplace_back();
                    poolSize.type = type;
                    poolSize.descriptorCount = static_cast<uint32_t>(DESCRIPTOR_POOL_SIZE * count);
                }

                VkDescriptorPoolCreateInfo poolInfo{};
                poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
                poolInfo.pPoolSizes = poolSizes.data();
                poolInfo.maxSets = static_cast<uint32_t>(DESCRIPTOR_POOL_SIZE);
                poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

                auto& pools = descriptorPoolDatas[key];
                auto& newPoolData = pools.emplace_back();

                if (vkCreateDescriptorPool(Instance()->_vDevice, &poolInfo, nullptr, &newPoolData.vDescriptorPool) != VK_SUCCESS) {
                    Error("failed to create descriptor pool!");
                }

                return newPoolData;
            }
            DescriptorPoolData& GetDescriptorPoolData(const PoolKey & key) {
                auto& pools = descriptorPoolDatas[key];

                for (auto& poolData : pools) {
                    if (poolData.count < DESCRIPTOR_POOL_SIZE) {
                        return poolData;
                    }
                }

                return CreateDescriptorPoolData(key);
            }

            void CleanDescriptorPools() {
                for (auto& pools : descriptorPoolDatas) {
                    for (auto& poolData : pools.second) {
                        poolData.count = 0;
                        vkResetDescriptorPool(Instance()->_vDevice, poolData.vDescriptorPool, 0);
                    }
                }
            }

            VkSemaphore vRenderFinishedSemaphore;
            VkFence vInFlightFence;

            std::vector<std::unordered_map<uint16_t, Uniform>> uniforms;
            std::unordered_map<PoolKey, std::vector<DescriptorPoolData>> descriptorPoolDatas;

            // TODO remake commandBuffer through acquiring directly from pool to reusing
            VkCommandBuffer vCommandBuffer;
        };

        virtual size_t GetProperImageDataIndex() const {
            for (size_t i = 0; i < _imageDatas.size(); ++i) {
                // TODO find proper image
                return i;
            }

            return _imageDatas.size();
        }

        size_t ChooseImageData() {
            return _imageDataIndex = GetProperImageDataIndex();
        }

        SingleImageData& PrepareImageData() {
            auto imageDataIndex = ChooseImageData();
            
            while (_imageDataIndex >= _imageDatas.size()) {
                _imageDatas.emplace_back(Instance()).Load();
            }

            auto& imageData = _imageDatas[imageDataIndex];

            vkWaitForFences(Instance()->_vDevice, 1, &imageData.vInFlightFence, VK_TRUE, UINT64_MAX);

            imageData.uniforms.clear();

            imageData.CleanDescriptorPools();

            {
                std::lock_guard<std::recursive_mutex> lg(Instance()->_deviceLock);
                vkResetFences(Instance()->_vDevice, 1, &imageData.vInFlightFence);
            }

            vkResetCommandBuffer(imageData.vCommandBuffer, 0);

            return imageData;
        }


        void SetRenderer(Renderer::Ptr renderer) {
            _renderer = renderer;
            _renderPass = renderer->renderPass();
            _pushConstants.clear();
            if (auto shader = renderer->meta()->_vertShader; shader != nullptr && shader->meta()->constant.SizeOf() > 0) {
                _pushConstants.try_emplace(VK_SHADER_STAGE_VERTEX_BIT, shader->meta()->constant, shader->meta()->constantOffset);
            }
            if (auto shader = renderer->meta()->_fragShader; shader != nullptr && shader->meta()->constant.SizeOf() > 0) {
                _pushConstants.try_emplace(VK_SHADER_STAGE_FRAGMENT_BIT, shader->meta()->constant, shader->meta()->constantOffset);
            }

            _uniformDatas.clear();
            renderer->meta()->populateUnifomDatas(_uniformDatas);
        }
        void ResetRenderer() {
            _renderer.Reset();
            _pushConstants.clear();
        }

        void Sampler(uint16_t set, uint16_t binding, std::shared_ptr<Sampler> sampler) {
            if (!_renderer.HasFreshValue() || !_renderer.FreshValue() || !_renderer.FreshValue()->IsLoaded()) {
                return;
            }

            auto& imageData = _imageDatas[_imageDataIndex];
            auto& renderer = _renderer.FreshValue();
            auto& setLayout = renderer->getSetLayout(set);
            auto& currentUniforms = imageData.uniforms.back();

            auto& uniform = currentUniforms.try_emplace(set,
                Instance(), imageData.GetDescriptorPoolData(setLayout._types), setLayout).first->second;

            auto& uniformData = uniform.setters.try_emplace(binding,
                std::make_shared<SamplerUniformSetter>(
                    uniform.vDescriptorSet, binding, sampler)
            ).first->second;
        }

        void SampledTexture(uint16_t set, uint16_t binding, std::shared_ptr<Rise::Sampler> sampler, std::shared_ptr<IImage> image) {
            if (!_renderer.HasFreshValue() || !_renderer.FreshValue() || !_renderer.FreshValue()->IsLoaded()) {
                return;
            }

            auto& imageData = _imageDatas[_imageDataIndex];
            auto& renderer = _renderer.FreshValue();
            auto& setLayout = renderer->getSetLayout(set);
            auto& currentUniforms = imageData.uniforms.back();

            auto& uniform = currentUniforms.try_emplace(set,
                Instance(), imageData.GetDescriptorPoolData(setLayout._types), setLayout).first->second;

            auto& uniformData = uniform.setters.try_emplace(binding,
                std::make_shared<ImageUniformSetter>(
                    uniform.vDescriptorSet, binding, image, sampler)
            ).first->second;
        }

        void Textures(uint16_t set, uint16_t binding, const std::vector<std::shared_ptr<IImage>>& images) {
            if (!_renderer.HasFreshValue() || !_renderer.FreshValue() || !_renderer.FreshValue()->IsLoaded()) {
                return;
            }

            auto& imageData = _imageDatas[_imageDataIndex];
            auto& renderer = _renderer.FreshValue();
            auto& setLayout = renderer->getSetLayout(set);
            auto& currentUniforms = imageData.uniforms.back();

            auto& uniform = currentUniforms.try_emplace(set,
                Instance(), imageData.GetDescriptorPoolData(setLayout._types), setLayout).first->second;

            auto& uniformData = uniform.setters.try_emplace(binding,
                std::make_shared<ImagesUniformSetter>(
                    uniform.vDescriptorSet, binding, images)
            ).first->second;
        }

        template <class T>
        void Uniform(uint16_t set, uint16_t binding, const std::string& id, const T& value) {
            if (!_renderer.HasFreshValue() || !_renderer.FreshValue() || !_renderer.FreshValue()->IsLoaded()) {
                return;
            }

            auto& renderer = _renderer.FreshValue();
            auto& metadata = renderer->meta()->metadata(set, binding);

            auto& uniformData = _uniformDatas.find(CombinedKey<uint16_t, uint16_t>(set, binding))->second;

            metadata.InsertValueAt(uniformData.data(), 0, id, value);
        }

        template <class T>
        void PushConstant(const std::string& id, const T& value) {
            for (auto& constantPair : _pushConstants) {
                auto& constant = constantPair.second;
                if (constant.metadata.InsertValueAt(constant.data.data(), 0, id, value)) {
                    break;
                }
            }
        }

        void SetViewport(const Square2D& viewport) {
            _viewport = viewport;
        }
        void ResetViewport() {
            _viewport.Reset();
        }

        void SetScissor(const Square2D& scissor) {
            _scissor = scissor;
        }
        void ResetScissor() {
            _scissor.Reset();
        }

        void SetVertices(std::shared_ptr<IVertices> vertices) {
            _vertices = vertices;
        }
        void ResetVertices() {
            _vertices.Reset();
        }

        void DrawImpl(uint32_t count) {
            auto& imageData = _imageDatas[_imageDataIndex];

            if (auto value = _renderer.CheckValue()) {
                auto renderer = *value;
                if (!renderer) {
                    return;
                }

                if (auto renderPassValue = _renderPass.CheckValue()) {
                    if (_renderPass.HasValue() && _renderPass.Value()) {
                        _renderPass.Value()->EndRecordCommandBuffer(imageData.vCommandBuffer);
                    }
                    auto renderPass = *renderPassValue;
                    if (renderPass) {
                        renderPass->StartRecordCommandBuffer(imageData.vCommandBuffer, *CurrentImage());
                    }
                    renderPassValue.ready();
                }

                if (!renderer->IsLoaded()) {
                    return;
                }

                _viewport.CurrentReset();
                _scissor.CurrentReset();
                _vertices.CurrentReset();

                renderer->BindPipeline(imageData.vCommandBuffer, imageMeta()->size);
                value.ready();
            }

            if (!_renderer.HasValue() || !_renderer.Value()->IsLoaded()) {
                return;
            }

            if (_renderer.Value()->meta()->_verticesFormat.SizeOf() > 0) {
                if (auto value = _vertices.CheckValue()) {
                    auto vertices = *value;
                    if (vertices && vertices->meta().format() == _renderer.Value()->meta()->_verticesFormat && vertices->IsLoaded()) {

                        VkBuffer vertexBuffers[] = { vertices->buffer().vBuffer() };
                        VkDeviceSize offsets[] = { 0 };
                        vkCmdBindVertexBuffers(imageData.vCommandBuffer, 0, 1, vertexBuffers, offsets);
                        value.ready();
                    }
                    else {
                        return;
                    }
                }
                else {
                    return;
                }
            }

            if (auto value = _viewport.CheckValue()) {
                if (
                    value->pos.x < 0 || value->pos.x >= static_cast<int32_t>(GetSize().width) ||
                    value->pos.y < 0 || value->pos.y >= static_cast<int32_t>(GetSize().height) ||
                    value->size.width == 0 || value->pos.x + value->size.width > GetSize().width ||
                    value->size.height == 0 || value->pos.y + value->size.height > GetSize().height
                    ) {
                    return;
                }

                VkViewport viewport{};
                viewport.x = static_cast<float>(value->pos.x);
                viewport.y = static_cast<float>(value->pos.y);
                viewport.width = static_cast<float>(value->size.width);
                viewport.height = static_cast<float>(value->size.height);
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;

                vkCmdSetViewport(imageData.vCommandBuffer, 0, 1, &viewport);
                value.ready();
            }

            if (auto value = _scissor.CheckValue()) {
                if (
                    value->pos.x < 0 || value->pos.x >= static_cast<int32_t>(GetSize().width) ||
                    value->pos.y < 0 || value->pos.y >= static_cast<int32_t>(GetSize().height) ||
                    value->size.width == 0 || value->pos.x + value->size.width > GetSize().width ||
                    value->size.height == 0 || value->pos.y + value->size.height > GetSize().height
                    ) {
                    return;
                }

                VkRect2D scissor{};
                scissor.offset = value->pos;
                scissor.extent = value->size;

                vkCmdSetScissor(imageData.vCommandBuffer, 0, 1, &scissor);
                value.ready();
            }

            auto& renderer = _renderer.Value();
            for (auto& [flag, constant] : _pushConstants) {
                renderer->PushConstant(imageData.vCommandBuffer, flag, constant.offset, constant.data);
            }

            auto& currentUniforms = imageData.uniforms.back();

            for (auto& [key, data] : _uniformDatas) {
                const auto& set = key.Get<0>();
                const auto& binding = key.Get<1>();

                auto& setLayout = renderer->getSetLayout(set);

                auto& uniform = currentUniforms.try_emplace(set,
                    Instance(), imageData.GetDescriptorPoolData(setLayout._types), setLayout).first->second;

                uniform.setters.try_emplace(binding,
                    std::make_shared<BufferUniformSetter>(
                        uniform.vDescriptorSet, binding, data));
            }

            for (auto& uniformPair : imageData.uniforms.back()) {
                if (!uniformPair.second.Ready()) {
                    return;
                }
                _renderer.Value()->BindDescriptorSet(imageData.vCommandBuffer, uniformPair.second);
            }

            vkCmdDraw(imageData.vCommandBuffer, count, 1, 0, 0);

            if (!imageData.uniforms.back().empty()) {
                imageData.uniforms.emplace_back();
            }
        }

        void ResetDrawResources() {
            _renderPass.FullReset();
            _renderer.FullReset();
            _viewport.FullReset();
            _scissor.FullReset();
            _vertices.FullReset();
        }

        std::vector<SingleImageData> _imageDatas;
        size_t _imageDataIndex;

        uint64_t _currentGlobalFrame = std::numeric_limits<uint64_t>::max();

        Dirty<std::shared_ptr<RenderPass>> _renderPass;
        Dirty< std::shared_ptr<Renderer>> _renderer;
        Dirty<Square2D> _viewport;
        Dirty<Square2D> _scissor;
        Dirty<std::shared_ptr<IVertices>> _vertices;

        struct PushConstantData {
            PushConstantData(const NamedMetadata<BaseType>& metadata_, uint32_t offset_)
                : metadata(metadata_), data(metadata_.SizeOf()), offset(offset_) {}

            NamedMetadata<BaseType> metadata;
            std::vector<uint8_t> data;
            uint32_t offset;
        };

        std::unordered_map<CombinedKey<uint16_t, uint16_t>, std::vector<uint8_t>> _uniformDatas;

        std::unordered_map<VkShaderStageFlags, PushConstantData> _pushConstants;

    };

    class RenderTarget : public IRenderTarget {
    public:

        std::vector<TargetImage> _images;

    };

}

#endif /* render_target_h */
