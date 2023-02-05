//☀Rise☀
#include "Rise/draw.h"

#include "Rise/render_target.h"
#include "Rise/vertices.h"
#include "Rise/framebuffer.h"

namespace Rise {

	namespace Draw {
		Context::Context(IRenderTarget& target, VkCommandBuffer commandBuffer, IImage* image) {
			_contextData.Set<ContextData::target>(&target);
			_contextData.Set<ContextData::commandBuffer>(commandBuffer);
			_contextData.Set<ContextData::image>(image);
			_contextData.Set<ContextData::vertices>(nullptr);
			_contextData.Set<ContextData::vertexCount>(0);

			Square2D defaultSquare({ 0, 0 }, image->GetMeta()->size);
			_contextData.Set<ContextData::scissor>(defaultSquare);
			_contextData.Set<ContextData::viewport>(defaultSquare);
		}

		Context::Context(const Context& context)
			: _contextData(context._contextData) {}

		Context::Context(Context&& context)
			: _contextData(context._contextData) {}

		Context::~Context() {
			if (_contextData.LocalExist<ContextData::renderer>()) {
				if (auto* parent = _contextData.Parent(); parent && parent->Exist<ContextData::renderer>()) {
					BindRenderer(parent->Get<ContextData::renderer>());
				}
				else {
					_contextData.Get<ContextData::target>()->ResetRenderer();
				}
			}

			if (_contextData.LocalExist<ContextData::vertices>()) {
				if (auto* parent = _contextData.Parent(); parent && parent->Exist<ContextData::vertices>()) {
					BindVertices(parent->Get<ContextData::vertices>());
				}
				else {
					_contextData.Get<ContextData::target>()->ResetVertices();
				}
			}

			if (_contextData.LocalExist<ContextData::viewport>()) {
				if (auto* parent = _contextData.Parent()) {
					SetViewport(parent->Get<ContextData::viewport>());
				}
				else {
					_contextData.Get<ContextData::target>()->ResetViewport();
				}
			}

			if (_contextData.LocalExist<ContextData::scissor>()) {
				if (auto* parent = _contextData.Parent()) {
					SetScissor(parent->Get<ContextData::scissor>());
				}
				else {
					_contextData.Get<ContextData::target>()->ResetScissor();
				}
			}
		}

		void Context::BindRenderer(std::shared_ptr<Renderer> renderer) {
			_contextData.Set<ContextData::renderer>(renderer);
			_contextData.Get<ContextData::target>()->SetRenderer(renderer);
		}

		const IImage* Context::GetTargetImageView() const {
			return _contextData.Get<ContextData::image>();
		}

		uint32_t Context::GetWidth() const {
			return GetTargetImageView()->GetMeta()->size.width;
		}

		uint32_t Context::GetHeight() const {
			return GetTargetImageView()->GetMeta()->size.height;
		}

		void Context::BindVertices(std::shared_ptr<IVertices> vertices) {
			_contextData.Get<ContextData::target>()->SetVertices(vertices);
			if (vertices) {
				SetVertexCount(vertices->meta().count());
			}
		}

		void Context::Sampler(uint16_t set, uint16_t binding, std::shared_ptr<Rise::Sampler> sampler) {
			_contextData.Get<ContextData::target>()->Sampler(set, binding, sampler);
		}

		void Context::SampledTexture(uint16_t set, uint16_t binding, std::shared_ptr<Rise::Sampler> sampler, std::shared_ptr<IImage> image) {
			_contextData.Get<ContextData::target>()->SampledTexture(set, binding, sampler, image);
		}

		void Context::Textures(uint16_t set, uint16_t binding, const std::vector<std::shared_ptr<IImage>>& images) {
			_contextData.Get<ContextData::target>()->Textures(set, binding, images);
		}

		void Context::Uniform(uint16_t set, uint16_t binding, const std::string& id, const glm::vec2& vec) {
			_contextData.Get<ContextData::target>()->Uniform(set, binding, id, vec);
		}

		void Context::PushConstant(const std::string& id, float value) {
			_contextData.Get<ContextData::target>()->PushConstant(id, value);
		}

		void Context::PushConstant(const std::string& id, uint32_t value) {
			_contextData.Get<ContextData::target>()->PushConstant(id, value);
		}

		void Context::PushConstant(const std::string& id, const glm::vec2& vec) {
			_contextData.Get<ContextData::target>()->PushConstant(id, vec);
		}

		void Context::PushConstant(const std::string& id, const glm::vec3& vec) {
			_contextData.Get<ContextData::target>()->PushConstant(id, vec);
		}

		void Context::PushConstant(const std::string& id, const glm::mat3& mat) {
			_contextData.Get<ContextData::target>()->PushConstant(id, mat);
		}

		void Context::SetViewport(const Square2D& square) {
			_contextData.Set<ContextData::viewport>(square);
			_contextData.Get<ContextData::target>()->SetViewport(square);
		}
		void Context::SetScissor(const Square2D& square) {
			_contextData.Set<ContextData::scissor>(square);
			_contextData.Get<ContextData::target>()->SetScissor(square);
		}

		void Context::SetVertexCount(uint32_t count) {
			_contextData.Set<ContextData::vertexCount>(count);
		}

		void Context::Draw(uint32_t vertexCount) {
			if (vertexCount > 0) {
				SetVertexCount(vertexCount);
			}
			_contextData.Get<ContextData::target>()->DrawImpl( 
				_contextData.Get<ContextData::vertexCount>());
		}
	}

	void drawSmth(Draw::Context context, const glm::vec2& pos, const glm::vec3& color) {
		context.BindRenderer(Rise::Instance()->ResourceGenerator().Get<Renderer>("default"));
		context.BindVertices(Rise::Instance()->ResourceGenerator().Get<Vertices>("funny_triangle"));
		context.Uniform(0, 0, "pos", pos);
		context.PushConstant("color", color);
		context.Draw();
	}

	void drawSmth(Draw::Context context, std::shared_ptr<IVertices> vertices, const glm::vec2& pos, const glm::vec3& color) {
		context.BindRenderer(Rise::Instance()->ResourceGenerator().Get<Renderer>("default"));
		context.BindVertices(vertices);
		context.Uniform(0, 0, "pos", pos);
		context.PushConstant("color", color);
		context.Draw();
	}
	void drawVerticesWired(Draw::Context context, std::shared_ptr<IVertices> vertices, const ColorRGB& color) {
		context.BindRenderer(Rise::Instance()->ResourceGenerator().Get<Renderer>("default_wired_line_strip"));
		context.BindVertices(vertices);
		context.PushConstant("color", color);
		context.Draw();
	}

	void drawSquare(Draw::Context context, const Square2D& square, const ColorRGB& color) {
		context.BindRenderer(Rise::Instance()->ResourceGenerator().Get<Renderer>("drawSquare"));

		context.SetScissor(square);
		context.SetViewport(square);

		context.PushConstant("color", color);
		context.Draw(4);
	}

	void drawTexture(Draw::Context context, const Square2D& square, std::shared_ptr<IImage> texture) {
		context.BindRenderer(Rise::Instance()->ResourceGenerator().Get<Renderer>("drawTexture"));

		context.SetScissor(square);
		context.SetViewport(square);

		context.SampledTexture(0, 0, Rise::Instance()->ResourceGenerator().GetByKey<Rise::Sampler>(CombinedKey<Sampler::Attachment>(Sampler::Attachment::Color)), texture);
		context.Draw(4);
	}

	void drawN9Slice(Draw::Context context, const Square2D& square, float scale, std::shared_ptr<N9Slice> texture) {
		context.BindRenderer(Rise::Instance()->ResourceGenerator().Get<Renderer>("draw9Slice"));

		context.SetScissor(square);
		context.SetViewport(square);

		context.SampledTexture(0, 0, Rise::Instance()->ResourceGenerator().GetByKey<Rise::Sampler>(CombinedKey<Sampler::Attachment>(Sampler::Attachment::Color)), texture->texture());
		
		context.PushConstant("leftPos", scale * texture->meta()->metrics.left / square.size.width);
		context.PushConstant("rightPos", (1 - scale * texture->meta()->metrics.right / square.size.width));
		context.PushConstant("topPos", scale * texture->meta()->metrics.top / square.size.height);
		context.PushConstant("bottomPos", (1 - scale * texture->meta()->metrics.bottom / square.size.height));

		context.PushConstant("leftText", static_cast<float>(texture->meta()->metrics.left) / texture->texture()->GetMeta()->size.width);
		context.PushConstant("rightText", (1 - static_cast<float>(texture->meta()->metrics.right) / texture->texture()->GetMeta()->size.width));
		context.PushConstant("topText", static_cast<float>(texture->meta()->metrics.top) / texture->texture()->GetMeta()->size.height);
		context.PushConstant("bottomText", (1 - static_cast<float>(texture->meta()->metrics.bottom) / texture->texture()->GetMeta()->size.height));
		
		context.Draw(4);
	}

}
