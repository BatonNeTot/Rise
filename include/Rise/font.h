//☀Rise☀
#ifndef font_h
#define font_h

#include "image.h"
#include "draw.h"

#include <unordered_map>

namespace Rise {
    
    class Font : public ResourceBase, public RiseObject {
    public:

        using Ptr = std::shared_ptr<Font>;
        constexpr static const char* Ext = "ttf";

        struct Meta {
            uint32_t glyphSize;

            void Setup(const Data& data) {
                glyphSize = data["glyphSize"].as(128u);
            }
        };

        Font(Core* core, Meta* meta)
            : RiseObject(core), _meta(meta) {}
        virtual ~Font() {
            if (IsLoaded()) {
                Unload();
            }
        }

        const Meta* GetMeta() const {
            return _meta;
        }

        struct GlyphInfo {
            Point2D offset;
            Point2D nextAnchor;
            FPoint2D atlasOffset;
            FSize2D atlasSize;
            Size2D size;
        };

        std::shared_ptr<IImage> GetGlyphAtlas() const {
            return _glyphAtlas;
        }

        const GlyphInfo& GetGlyphInfo(uint64_t symbol) const;

        Size2D getTextBounding(const std::string& text, uint32_t size) const;

        void LoadFromFile(const std::string& filename);

        void Unload() override;

    private:

        const Meta* _meta = nullptr;

        std::vector<GlyphInfo> _glyphs;
        std::unordered_map<uint64_t, uint32_t> _charmap;

        CustomImage::Meta _atlasMeta;
        std::shared_ptr<IImage> _glyphAtlas;
    };

    namespace Draw {

        void drawText(Draw::Context context, const std::string& text, const Point2D& point, std::shared_ptr<Font> font, uint32_t size);
        void drawText(Draw::Context context, const std::string& text, const Point2D& point, std::shared_ptr<Font> font, uint32_t size, const ColorRGB& color);

    }

}

#endif /* font_h */
