//☀Rise☀
#include "9slice_ncomponent.h"

#include "resource_manager.h"

namespace Rise {

    REGISTER_NCOMPONENT(N9SliceNComponent);

    N9SliceNComponent::N9SliceNComponent(Core* core)
        : NComponent(core) {}
    N9SliceNComponent::N9SliceNComponent(Core* core, const Data& config)
        : NComponent(core) {
        _texture = Rise::Instance()->ResourceGenerator().Get<N9Slice>(config["texture"]);
    }
    N9SliceNComponent::~N9SliceNComponent() {}

    void N9SliceNComponent::draw(Draw::Context context) {
        if (!_texture) {
            return;
        }
        if (!_texture->texture()) {
            return;
        }

        if (_scale == 0.f) {
            auto scaleX = static_cast<float>(GetSize().width / _texture->texture()->GetMeta()->size.width);
            auto scaleY = static_cast<float>(GetSize().height / _texture->texture()->GetMeta()->size.height);
            drawN9Slice(context, { GetPos(), GetSize() }, (scaleX + scaleY) / 2, _texture);
        } else {
            drawN9Slice(context, { GetPos(), GetSize() }, _scale, _texture);
        }
    }

}
