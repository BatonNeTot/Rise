//☀Rise☀
#ifndef texture_ncomponent_h
#define texture_ncomponent_h

#include "Rise/node/node.h"

#include "Rise/image.h"

namespace Rise {

    class TextureNComponent : public NComponent {
    public:
        TextureNComponent(Core* core);
        TextureNComponent(Core* core, const Data& config);
        ~TextureNComponent() override;

        void setTexture(std::shared_ptr<IImage> texture) {
            _texture = texture;
        }

    private:
        bool canDraw() const override { return true; }

        void draw(Draw::Context context) override;

        std::shared_ptr<IImage> _texture;
    };

}

#endif /* texture_ncomponent_h */
