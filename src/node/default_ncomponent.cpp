//☀Rise☀
#include "Rise/node/default_ncomponent.h"

#include "Rise/draw.h"
#include "Rise/image.h"
#include "Rise/font.h"

#include <chrono>
#define _USE_MATH_DEFINES
#include <math.h>

namespace Rise {

    REGISTER_NCOMPONENT(DefaultNComponent)

    DefaultNComponent::DefaultNComponent(Core* core, const Data& config)
        : NComponent(core), _color(config["color"]) {
        _vertices = Rise::Instance()->Resources().CreateRes<CustomVertices>();

        Rise::Instance()->Loader().AddJob([this]() {
            Bezie<3> bezie(
                FPoint2D(-1.f, 1.f),
                FPoint2D(1.f, 1.f),
                FPoint2D(1.f, -1.f)
            );

            std::vector<glm::vec2> points;
            const auto total = 100;
            for (auto i = 0; i <= total; ++i) {
                const auto t = static_cast<float>(i) / total;
                points.emplace_back(bezie.calculate(t));
            }

            CustomVertices::Meta meta(static_cast<uint32_t>(points.size()), { VerticesType::Vec2 });
            _vertices->SetupMeta(meta);

            std::vector<uint8_t> data(meta.sizeOf());

            for (uint32_t i = 0; i < points.size(); ++i) {
                meta.format().InsertValueAt(data.data(), i, 0, points[i]);
            }

            _vertices->Load(data);
            });
    }
    DefaultNComponent::~DefaultNComponent() {}

    void DefaultNComponent::draw(Draw::Context context) {

        static auto start = std::chrono::system_clock::now();
        auto current = std::chrono::system_clock::now();
        auto time = std::chrono::duration<float>(current - start).count();
        auto roundedTime = time;
        auto posY = std::cosf(roundedTime);

        const auto& pos = GetPos();
        const auto& quarter = GetSize() / 4;
        drawSquare(context, { pos + quarter, quarter * 2 }, _color);
        drawTexture(context, { {
                pos.x + static_cast<int32_t>(quarter.width),
                pos.y + static_cast<int32_t>(quarter.height) * 2
            }, quarter }, Instance()->ResourceGenerator().Get<Image>("texture"));
        drawSmth(context, { 0.5, -posY }, { 0.0,1.0,1.0 });

        drawVerticesWired(context, _vertices, { 255, 0, 255 });

        drawText(context, "Test!", {
            pos.x + static_cast<int32_t>(quarter.width),
            pos.y + static_cast<int32_t>(quarter.height) * 2
            }, Instance()->ResourceGenerator().Get<Font>("times_new_roman"),
            128);
    }

}
