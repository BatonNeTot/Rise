//☀Rise☀
#include "Rise/node/button_ncomponent.h"

namespace Rise {

    REGISTER_NCOMPONENT(ButtonNComponent)

        ButtonNComponent::ButtonNComponent(Core* core)
        : NComponent(core) {}
    ButtonNComponent::ButtonNComponent(Core* core, const Data& config)
        : NComponent(core) {
    }
    ButtonNComponent::~ButtonNComponent() {}


    bool ButtonNComponent::onMouseClick(bool leftKey, const Point2D& point) {
        _action(leftKey);
        return true;
    }

}
