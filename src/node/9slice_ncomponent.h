//☀Rise☀
#ifndef n9slice_ncomponent_h
#define n9slice_ncomponent_h

#include "node.h"

#include "image.h"

namespace Rise {

    class N9SliceNComponent : public NComponent {
    public:
        N9SliceNComponent(Core* core);
        N9SliceNComponent(Core* core, const Data& config);
        ~N9SliceNComponent() override;

        void setTexture(std::shared_ptr<N9Slice> texture) {
            _texture = texture;
        }

        void setScale(float scale) {
            _scale = scale;
        }

    private:
        bool canDraw() const override { return true; }

        void draw(Draw::Context context) override;

        float _scale = 1.f;
        std::shared_ptr<N9Slice> _texture;
    };

}

#endif /* n9slice_ncomponent_h */
