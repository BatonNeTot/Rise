//☀Rise☀
#ifndef render_pass_h
#define render_pass_h

#include "vertices.h"
#include "image.h"
#include "framebuffer.h"

#include <vulkan/vulkan.h>

#include <vector>
#include <unordered_map>

namespace Rise {

    class RenderPass;
    class Uniform;

    class IShader;

    class Renderer : public ResourceBase, public RiseObject {
    public:

        using Ptr = std::shared_ptr<Renderer>;
        constexpr static const char* Ext = "rndr";

        constexpr static bool MetaOnly = true;

        class Meta {
        public:
            void Setup(const Data& data);

            void populateUnifomDatas(std::unordered_map<CombinedKey<uint16_t, uint16_t>, std::vector<uint8_t>>& uniforms) const;
            const NamedMetadata<>& metadata(uint16_t set, uint16_t binding) const;

            std::shared_ptr<Rise::IShader> _vertShader = nullptr;
            std::shared_ptr<Rise::IShader> _fragShader = nullptr;

            Rise::VerticesFormat _verticesFormat;

            bool _wired;
            bool _lineStrip;
            bool _useAlpha;
        private:
        };

        Renderer(Core* core, Meta* meta);
        ~Renderer();

        struct SetLayout {
            std::unordered_map<VkDescriptorType, uint32_t> _types;
            VkDescriptorSetLayout _vDescriptorSetLayout;
        };

        const SetLayout& getSetLayout(uint16_t set) const;

        void BindPipeline(VkCommandBuffer commandBuffer, const VkExtent2D& vExtent);

        void BindDescriptorSet(VkCommandBuffer commandBuffer, Uniform& uniform);
        void PushConstant(VkCommandBuffer commandBuffer, VkShaderStageFlags stageFlags, uint32_t offset, const std::vector<uint8_t>& data);

        const Meta* meta() {
            return _meta;
        }

        std::shared_ptr<RenderPass> renderPass() const {
            return _renderPass;
        }

        void Load();

    protected:

        void Unload() override;

    private:
        void CreatePipelineLayout(VkPipelineLayout& layout);

        VkPipelineLayout _vPipelineLayout;
        VkPipeline _vPipeline = nullptr;

        std::vector<SetLayout> _setLayouts;

        Meta* _meta;
        std::shared_ptr<Rise::RenderPass> _renderPass;
    };

    class Window;

    class RenderPass : public ResourceBase, public RiseObject {
    public:

        enum class Attachment {
            Color
        };

        using Ptr = std::shared_ptr<RenderPass>;
        constexpr static bool InstantLoad = true;

        friend Renderer;
        friend Window;

        explicit RenderPass(Core* core);
        ~RenderPass();

        void Load(const CombinedValue<std::vector<RenderPass::Attachment>>& value);

        void StartRecordCommandBuffer(VkCommandBuffer commandBuffer, IImage& image);
        void EndRecordCommandBuffer(VkCommandBuffer commandBuffer);

        VkRenderPass VRenderPass() const {
            return _vRenderPass;
        }

    private:

        VkRenderPass _vRenderPass = nullptr;
    };

}

#endif /* render_pass_h */
