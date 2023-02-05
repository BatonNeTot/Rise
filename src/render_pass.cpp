//☀Rise☀
#include "Rise/render_pass.h"

#include "Rise/uniform.h"
#include "Rise/window.h"
#include "Rise/rise.h"

namespace Rise {

    Renderer::Renderer(Core* core, Meta* meta)
        : RiseObject(core), _meta(meta) {
        auto attachments = std::vector<RenderPass::Attachment>({ RenderPass::Attachment::Color });
        _renderPass = Rise::Instance()->ResourceGenerator().GetByKey<RenderPass>(
            CombinedKey<std::vector<RenderPass::Attachment>>(attachments));
    }

    void Renderer::Meta::Setup(const Data& data) {
        _vertShader = Rise::Instance()->ResourceGenerator().Get<IShader>(data["vertShader"].as<std::string>());
        _fragShader = Rise::Instance()->ResourceGenerator().Get<IShader>(data["fragShader"].as<std::string>());

        _wired = data["wired"].as<std::string>() == "true";
        _lineStrip = data["polygon"].as<std::string>() == "line_strip";
        _useAlpha = data["useAlpha"].as<bool>(false);

        {
            std::vector<VerticesType> types;

            for (auto typeStr : data["format"]) {
                types.emplace_back(VerticesType::FromString(typeStr.as<std::string>().c_str()));
            }

            _verticesFormat = { types };
        }
    }

    void Renderer::Meta::populateUnifomDatas(std::unordered_map<CombinedKey<uint16_t, uint16_t>, std::vector<uint8_t>>& uniforms) const {
        for (auto& uniform : _vertShader->meta()->uniforms) {
            auto& uniformData = uniform.second;
            if (uniformData.metadata.SizeOf() > 0) {
                uniforms.try_emplace(uniform.first, uniformData.metadata.SizeOf());
            }
        }
        for (auto& uniform : _fragShader->meta()->uniforms) {
            auto& uniformData = uniform.second;
            if (uniformData.metadata.SizeOf() > 0) {
                uniforms.try_emplace(uniform.first, uniformData.metadata.SizeOf());
            }
        }
    }

    const NamedMetadata<>& Renderer::Meta::metadata(uint16_t set, uint16_t binding) const {
        CombinedKey<uint16_t, uint16_t> key = { set, binding };

        {
            auto it = _vertShader->meta()->uniforms.find(key);
            if (it != _vertShader->meta()->uniforms.end()) {
                return it->second.metadata;
            }
        }

        {
            auto it = _fragShader->meta()->uniforms.find(key);
            if (it != _fragShader->meta()->uniforms.end()) {
                return it->second.metadata;
            }
        }

        static NamedMetadata<> defaultMetadata;
        return defaultMetadata;
    }

    Renderer::~Renderer() {
        if (IsLoaded()) {
            Unload();
        }
    }

    void Renderer::Load() {
        while (!_renderPass->IsLoaded()) {}
        CreatePipelineLayout(_vPipelineLayout);

        while (!meta()->_vertShader->IsLoaded()) {}
        auto vertModule = meta()->_vertShader->createModule();
        while (!meta()->_fragShader->IsLoaded()) {}
        auto fragModule = meta()->_fragShader->createModule();

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertModule.VShaderModule();
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragModule.VShaderModule();
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        std::vector<VkVertexInputBindingDescription> bindingDescriptions;
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

        if (meta()->_verticesFormat.SizeOf() > 0) {
            meta()->_verticesFormat.FillBindingDescription(bindingDescriptions);
            meta()->_verticesFormat.FillAttributeDescriptions(attributeDescriptions);
        }

        vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = meta()->_lineStrip ? VK_PRIMITIVE_TOPOLOGY_LINE_STRIP : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = 1.f;
        viewport.height = 1.f;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = { 1, 1 };

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = meta()->_wired ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        
        if (meta()->_useAlpha) {
            colorBlendAttachment.blendEnable = VK_TRUE;
            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        }
        else {
            colorBlendAttachment.blendEnable = VK_FALSE;
            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
            colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
            colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
            colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
        }

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        std::array<VkDynamicState, 2> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = nullptr; // Optional
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = _vPipelineLayout;
        pipelineInfo.renderPass = _renderPass->_vRenderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

        {
            std::lock_guard<std::recursive_mutex> lg(Instance()->_deviceLock);
            if (vkCreateGraphicsPipelines(Instance()->_vDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_vPipeline) != VK_SUCCESS) {
                Error("failed to create graphics pipeline!");
            }
        }

        MarkLoaded();
    }

    void Renderer::Unload() {
        for (const auto& layout : _setLayouts) {
            vkDestroyDescriptorSetLayout(Instance()->_vDevice, layout._vDescriptorSetLayout, nullptr);
        }
        _setLayouts.clear();

        vkDestroyPipelineLayout(Instance()->_vDevice, _vPipelineLayout, nullptr);
        vkDestroyPipeline(Instance()->_vDevice, _vPipeline, nullptr);

        MarkUnloaded();
    }

    const Renderer::SetLayout& Renderer::getSetLayout(uint16_t set) const {
        return _setLayouts[set];
    }

    void Renderer::BindPipeline(VkCommandBuffer commandBuffer, const VkExtent2D& vExtent) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _vPipeline);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)vExtent.width;
        viewport.height = (float)vExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);


        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = vExtent;

        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }

    void Renderer::CreatePipelineLayout(VkPipelineLayout& layout) {
        uint32_t maxSetNumber = 0;

        maxSetNumber = std::max(maxSetNumber, meta()->_vertShader->meta()->MaxSetNumber());
        maxSetNumber = std::max(maxSetNumber, meta()->_fragShader->meta()->MaxSetNumber());

        _setLayouts.resize(maxSetNumber);

        for (uint32_t i = 0; i < maxSetNumber; ++i) {
            auto& layout = _setLayouts[i];
            std::vector<VkDescriptorSetLayoutBinding> uboLayoutBindings;

            meta()->_vertShader->meta()->PopulateLayoutBindings(layout, uboLayoutBindings, i);
            meta()->_fragShader->meta()->PopulateLayoutBindings(layout, uboLayoutBindings, i);

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = static_cast<uint32_t>(uboLayoutBindings.size());
            layoutInfo.pBindings = uboLayoutBindings.data();

            if (vkCreateDescriptorSetLayout(Instance()->_vDevice, &layoutInfo, nullptr, &layout._vDescriptorSetLayout) != VK_SUCCESS) {
                Error("failed to create descriptor set layout!");
            }
        }

        std::vector<VkPushConstantRange> pushConstants;

        if (meta()->_vertShader->meta()->constant.SizeOf() > 0) {
            uint32_t offset = 0;
            if (!pushConstants.empty()) {
                offset = pushConstants.back().offset + pushConstants.back().size;
            }

            auto& pushConstant = pushConstants.emplace_back();
            pushConstant.offset = offset;
            pushConstant.size = meta()->_vertShader->meta()->constant.SizeOf();
            pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        }
        if (meta()->_fragShader->meta()->constant.SizeOf() > 0) {
            uint32_t offset = 0;
            if (!pushConstants.empty()) {
                offset = pushConstants.back().offset + pushConstants.back().size;
            }

            auto& pushConstant = pushConstants.emplace_back();
            pushConstant.offset = offset;
            pushConstant.size = meta()->_fragShader->meta()->constant.SizeOf();
            pushConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        }

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts(_setLayouts.size());
        for (auto i = 0; i < _setLayouts.size(); ++i) {
            descriptorSetLayouts[i] = _setLayouts[i]._vDescriptorSetLayout;
        }

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
        pipelineLayoutInfo.pPushConstantRanges = pushConstants.data();

        if (vkCreatePipelineLayout(Instance()->_vDevice, &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS) {
            Error("failed to create pipeline layout!");
        }
    }

    void Renderer::BindDescriptorSet(VkCommandBuffer commandBuffer, Uniform& uniform) {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _vPipelineLayout, 0, 1, &uniform.vDescriptorSet, 0, nullptr);
    }

    void Renderer::PushConstant(VkCommandBuffer commandBuffer, VkShaderStageFlags stageFlags, uint32_t offset, const std::vector<uint8_t>& data) {
        vkCmdPushConstants(commandBuffer, _vPipelineLayout, stageFlags, offset, static_cast<uint32_t>(data.size()), data.data());
    }

    RenderPass::RenderPass(Core* core)
        : RiseObject(core) {}
    RenderPass::~RenderPass() {
        vkDestroyRenderPass(Instance()->_vDevice, _vRenderPass, nullptr);
    }

    void RenderPass::Load(const CombinedValue<std::vector<RenderPass::Attachment>>& value) {
        VkAttachmentDescription colorAttachment{};
        // TODO remember and check if after recreation of a swap chain imgae format was changed
        // TODO get it from renderer
        colorAttachment.format = VK_FORMAT_B8G8R8A8_SRGB;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        {
            std::lock_guard<std::recursive_mutex> lg(Instance()->_deviceLock);
            if (vkCreateRenderPass(Instance()->_vDevice, &renderPassInfo, nullptr, &_vRenderPass) != VK_SUCCESS) {
                Error("failed to create render pass!");
            }
        }

        MarkLoaded();
    }

	void RenderPass::StartRecordCommandBuffer(VkCommandBuffer commandBuffer, IImage& imageView) {
        auto& framebuffer = imageView.getOrCreateFramebuffer(*this);

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = _vRenderPass;
        renderPassInfo.framebuffer = framebuffer.VFramebuffer();
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = framebuffer.image()->GetMeta()->size;

        VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        std::lock_guard<std::recursive_mutex> lg(Instance()->_deviceLock);
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void RenderPass::EndRecordCommandBuffer(VkCommandBuffer commandBuffer) {
        std::lock_guard<std::recursive_mutex> lg(Instance()->_deviceLock);
        vkCmdEndRenderPass(commandBuffer);
    }

}
