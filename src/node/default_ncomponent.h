//☀Rise☀
#ifndef default_ncomponent_h
#define default_ncomponent_h

#include "node.h"

#include "resource_manager.h"
#include "vertices.h"

namespace Rise {

    class DefaultNComponent : public NComponent {
    public:
        explicit DefaultNComponent(Core* core, const Data& config);
        ~DefaultNComponent() override;

    private:
        bool canDraw() const override { return true; }

        void draw(Draw::Context context) override;

        ColorRGB _color;
        CustomVertices::Ptr _vertices;
    };

}

#endif /* default_ncomponent_h */
