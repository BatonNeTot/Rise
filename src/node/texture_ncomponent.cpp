//☀Rise☀
#include "texture_ncomponent.h"

#include "resource_manager.h"

namespace Rise {

    REGISTER_NCOMPONENT(TextureNComponent)

    TextureNComponent::TextureNComponent(Core* core)
        : NComponent(core) {}
    TextureNComponent::TextureNComponent(Core* core, const Data& config)
        : NComponent(core) {
        _texture = Rise::Instance()->ResourceGenerator().Get<Image>(config["texture"]);
    }
    TextureNComponent::~TextureNComponent() {}

    void TextureNComponent::draw(Draw::Context context) {
        drawTexture(context, { GetPos(), GetSize() }, _texture);
    }

}
