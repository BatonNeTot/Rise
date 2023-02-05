//☀Rise☀
#ifndef label_ncomponent_h
#define label_ncomponent_h

#include "Rise/node/node.h"

#include "Rise/font.h"

namespace Rise {

    class LabelNComponent : public NComponent {
    public:
        LabelNComponent(Core* core);
        LabelNComponent(Core* core, const Data& config);
        ~LabelNComponent() override;

        void SetText(std::string&& text) {
            _text = std::move(text);
        }

        void SetFont(std::shared_ptr<Font> font) {
            _font = std::move(font);
        }

        void setSize(uint32_t size) {
            _size = size;
        }

    private:
        bool canDraw() const override { return true; }

        void draw(Draw::Context context) override;

        uint32_t _size = 32u;
        std::string _text;
        std::shared_ptr<Font> _font;
    };

}

#endif /* label_ncomponent_h */
