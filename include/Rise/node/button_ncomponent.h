//☀Rise☀
#ifndef button_ncomponent_h
#define button_ncomponent_h

#include "node.h"

namespace Rise {

    class ButtonNComponent : public NComponent {
    public:
        ButtonNComponent(Core* core);
        ButtonNComponent(Core* core, const Data& config);
        ~ButtonNComponent() override;

        void setAction(const std::function<void(bool)>& action) {
            _action = action;
        }

    private:

        bool onMouseClick(bool leftKey, const Point2D& point) override;

        std::function<void(bool)> _action;
    };

}

#endif /* texture_ncomponent_h */
