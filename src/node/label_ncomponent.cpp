//☀Rise☀
#include "Rise/node/label_ncomponent.h"

#include "Rise/resource_manager.h"

namespace Rise {

    REGISTER_NCOMPONENT(LabelNComponent);

    LabelNComponent::LabelNComponent(Core* core)
        : NComponent(core) {}
    LabelNComponent::LabelNComponent(Core* core, const Data& config)
        : NComponent(core) {
        _font = Rise::Instance()->ResourceGenerator().Get<Font>(config["font"]);
        _size = config["size"].as(_size);
    }
    LabelNComponent::~LabelNComponent() {}

    void LabelNComponent::draw(Draw::Context context) {
        auto point = GetPos();
        point.y += GetSize().height;
        Draw::drawText(context, _text, point, _font, _size);
    }

}
