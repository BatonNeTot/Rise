//☀Rise☀
#ifndef draw_h
#define draw_h

#include "render_pass.h"
#include "resource.h"
#include "types.h"

namespace Rise {

    class IRenderTarget;
    class Vertices;

    namespace Draw {
        class Context {
        public:

            friend IRenderTarget;

            Context(IRenderTarget& target, VkCommandBuffer commandBuffer, IImage* image);
            Context(const Context& context);
            Context(Context&& context);
            ~Context();

            void BindRenderer(std::shared_ptr<Renderer> renderer);

            void BindVertices(std::shared_ptr<IVertices> vertices);

            void Sampler(uint16_t set, uint16_t binding, std::shared_ptr<Rise::Sampler> sampler);
            void SampledTexture(uint16_t set, uint16_t binding, std::shared_ptr<Rise::Sampler> sampler, std::shared_ptr<IImage> image);
            void Textures(uint16_t set, uint16_t binding, const std::vector<std::shared_ptr<IImage>>& images);

            void Uniform(uint16_t set, uint16_t binding, const std::string& id, const glm::vec2& vec);

            void PushConstant(const std::string& id, float value);
            void PushConstant(const std::string& id, uint32_t value);
            void PushConstant(const std::string& id, const glm::vec2& vec);
            void PushConstant(const std::string& id, const glm::vec3& vec);
            void PushConstant(const std::string& id, const glm::mat3& vec);

            void SetScissor(const Square2D& square);
            void SetViewport(const Square2D& square);

            void SetVertexCount(uint32_t count);

            void Draw(uint32_t vertexCount = 0);

            const IImage* GetTargetImageView() const;

            uint32_t GetWidth() const;

            uint32_t GetHeight() const;

        private:

            CREATE_CLASS(ContextData, RecursiveValue,
                (target, commandBuffer, image, renderer, vertices, vertexCount, viewport, scissor),
                (IRenderTarget*, VkCommandBuffer, IImage*, std::shared_ptr<Renderer>, std::shared_ptr<IVertices>, uint32_t, Rise::Square2D, Rise::Square2D));

            ContextData _contextData;
        };
    }

	void drawSmth(Draw::Context context, const glm::vec2& pos, const glm::vec3& color);
	void drawSmth(Draw::Context context, std::shared_ptr<IVertices> vertices, const glm::vec2& pos, const glm::vec3& color);
    void drawVerticesWired(Draw::Context context, std::shared_ptr<IVertices> vertices, const ColorRGB& color);

    void drawSquare(Draw::Context context, const Square2D& square, const ColorRGB& color);
    void drawTexture(Draw::Context context, const Square2D& square, std::shared_ptr<IImage> texture);
    void drawN9Slice(Draw::Context context, const Square2D& square, float scale, std::shared_ptr<N9Slice> texture);

}

#endif /* draw_h */
