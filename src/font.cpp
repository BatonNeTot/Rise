//☀Rise☀
#include "font.h"

#include "rise.h"

#include FT_GLYPH_H

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

namespace Rise {

    void Font::LoadFromFile(const std::string& filename) {
        FT_Face face;
        static auto load_flags = FT_LOAD_DEFAULT;
        static auto render_mode = FT_RENDER_MODE_NORMAL;

        {
            auto error = FT_New_Face(Instance()->_freeTypeLibrary, filename.c_str(),
                0, &face);
            if (error == FT_Err_Unknown_File_Format)
            {
                Error("unknown free type format!");
            }
            else if (error)
            {
                Error("couldn't load free type file!");
            }
        }

        {
            auto error = FT_Select_Charmap(
                face,               /* target face object */
                FT_ENCODING_UNICODE); /* encoding           */
            if (error)
            {
                Error("couldn't select free type charmap!");
            }
        }

        auto glyphSize = GetMeta()->glyphSize;

        {
            auto error = FT_Set_Char_Size(
                face,    /* handle to face object           */
                0,       /* char_width in 1/64th of points  */
                16 * glyphSize,   /* char_height in 1/64th of points */
                300,     /* horizontal device resolution    */
                300); 
            if (error)
            {
                Error("couldn't set free type char size!");
            }
        }

        _glyphs = std::vector<GlyphInfo>(face->num_glyphs);

        std::vector<FT_BitmapGlyph> ftGlyphs(face->num_glyphs);
        std::vector<stbrp_rect> rects(face->num_glyphs);

        for (auto glyph_index = 0; glyph_index < face->num_glyphs; ++glyph_index) {
            {/* load glyph image into the slot (erase previous one) */
                auto error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
                if (error)
                {
                    Error("couldn't load free type glyph!");
                }
            }

            auto& glyphInfo = _glyphs[glyph_index];

            glyphInfo.nextAnchor.x = face->glyph->advance.x >> 6;
            glyphInfo.nextAnchor.y = face->glyph->advance.y >> 6;

            {
                auto error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_SDF);
                if (error)
                {
                    Error("couldn't render free type glyph!");
                }
            }

            FT_Glyph ftGlyph;
            {
                auto error = FT_Get_Glyph(face->glyph, &ftGlyph);
                if (error)
                {
                    Error("couldn't get free type glyph from slot!");
                }
            }

            ftGlyphs[glyph_index] = reinterpret_cast<FT_BitmapGlyph>(ftGlyph);
            
            rects[glyph_index].w = ftGlyphs[glyph_index]->bitmap.width;
            rects[glyph_index].h = ftGlyphs[glyph_index]->bitmap.rows;
        }

        uint32_t textureSize = 1;
        uint64_t sqrtN = static_cast<uint64_t>(std::ceil(std::sqrt(rects.size())));
        while (textureSize < sqrtN) {
            textureSize *= 2;
        }
        textureSize *= glyphSize * 2;

        stbrp_context rectContext;
        std::vector<stbrp_node> rectNodes(textureSize);

        stbrp_init_target(&rectContext, textureSize, textureSize, rectNodes.data(), static_cast<int>(rectNodes.size()));
        if (!stbrp_pack_rects(&rectContext, rects.data(), static_cast<int>(rects.size()))) {
            Error("couldn't pack free type glyph!");
        }

        _atlasMeta.size.width = textureSize;
        _atlasMeta.size.height = textureSize;
        _atlasMeta.vFormat = VK_FORMAT_R8_UNORM;
        auto customAtlas = Instance()->Resources().CreateRes<CustomImage>(&_atlasMeta);
        _glyphAtlas = customAtlas;

        constexpr auto pixelSize = 1;
        auto imageDataPtr = customAtlas->LoadPrepare(pixelSize);
        uint8_t* data = reinterpret_cast<uint8_t*>(imageDataPtr.ptr());
        auto bufferSize = textureSize * textureSize * pixelSize;

        for (auto i = 0u; i < bufferSize; ++i) {
            data[i] = 0;
        }

        for (auto glyph_index = 0; glyph_index < face->num_glyphs; ++glyph_index) {
            auto& rect = rects[glyph_index];
            auto& ftGlyph = ftGlyphs[glyph_index];
            auto& glyphInfo = _glyphs[glyph_index];

            glyphInfo.atlasOffset.x = static_cast<float>(rect.x) / textureSize;
            glyphInfo.atlasOffset.y = static_cast<float>(rect.y) / textureSize;

            glyphInfo.atlasSize.width = static_cast<float>(ftGlyph->bitmap.width) / textureSize;
            glyphInfo.atlasSize.height = static_cast<float>(ftGlyph->bitmap.rows) / textureSize;

            glyphInfo.offset.x = ftGlyph->left;
            glyphInfo.offset.y = -ftGlyph->top;

            glyphInfo.size.width = ftGlyph->bitmap.width;
            glyphInfo.size.height = ftGlyph->bitmap.rows;

            for (unsigned int j = 0u; j < ftGlyph->bitmap.rows; ++j) {
                for (unsigned int i = 0u; i < ftGlyph->bitmap.width; ++i) {
                    data[((rect.y + j) * textureSize + rect.x + i) * pixelSize + (pixelSize - 1)] = 
                        ftGlyph->bitmap.buffer[j * ftGlyph->bitmap.pitch + i];
                }
            }
        }

        customAtlas->LoadCommit(pixelSize);

        FT_ULong  charcode;
        FT_UInt   gindex;
        for (charcode = FT_Get_First_Char(face, &gindex); gindex != 0; charcode = FT_Get_Next_Char(face, charcode, &gindex)) {
            _charmap.try_emplace(charcode, gindex);
        }

        for (auto& ftGlyph : ftGlyphs) {
            FT_Done_Glyph(reinterpret_cast<FT_Glyph>(ftGlyph));
        }

        FT_Done_Face(face);

        MarkLoaded();
    }

    void Font::Unload() {
        _glyphs.clear();
        _glyphAtlas.reset();
        MarkUnloaded();
    }

    const Font::GlyphInfo& Font::GetGlyphInfo(uint64_t symbol) const {
        return _glyphs[_charmap.find(symbol)->second];
    }

    Size2D Font::getTextBounding(const std::string& text, uint32_t size) const {
        Point2D anchor;
        auto scale = static_cast<float>(size) / GetMeta()->glyphSize;

        for (auto& symbol : text) {
            auto& info = GetGlyphInfo(symbol);

            Square2D square = { anchor + info.offset * scale, info.size * scale };

            anchor += info.nextAnchor * scale;
        }

        return { 0, 0 };
    }

    void Draw::drawText(Draw::Context context, const std::string& text, const Point2D& point, std::shared_ptr<Font> font, uint32_t size) {
        drawText(std::move(context), text, point, font, size, ColorRGB(0, 0, 0));
    }

    void Draw::drawText(Draw::Context context, const std::string& text, const Point2D& point, std::shared_ptr<Font> font, uint32_t size, const ColorRGB& color) {
        if (!font->IsLoaded()) {
            return;
        }

        context.BindRenderer(Rise::Instance()->ResourceGenerator().Get<Renderer>("drawIndexedTexture"));

        context.SampledTexture(0, 0, Rise::Instance()->ResourceGenerator().GetByKey<Rise::Sampler>(CombinedKey<Sampler::Attachment>(Sampler::Attachment::Color)),
            font->GetGlyphAtlas());

        const auto topMargin = -static_cast<int32_t>(size * 0.1);
        const auto bottomMargin = static_cast<int32_t>(size * 0.2);

        Point2D anchor = { point.x, point.y };// +static_cast<int32_t>(size) + topMargin };
        auto scale = static_cast<float>(size) / font->GetMeta()->glyphSize;

        for (auto& symbol : text) {
            if (symbol == '\n') {
                anchor = { point.x, anchor.y + static_cast<int32_t>(size) + topMargin + bottomMargin };
                continue;
            }

            auto& info = font->GetGlyphInfo(symbol);

            Square2D square = { anchor + info.offset * scale, info.size * scale };
            context.SetScissor(square);
            context.SetViewport(square);

            context.PushConstant("offset", info.atlasOffset);
            context.PushConstant("size", info.atlasSize);
            context.PushConstant("textColor", color);

            context.Draw(4);

            anchor += info.nextAnchor * scale;
        }
    }

}
