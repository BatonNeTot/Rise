//☀Rise☀
#ifndef rectangle_node_h
#define rectangle_node_h

#include "node.h"

namespace Rise {

    class RectangleNode : public Node {
    public:

        RectangleNode(Core* core, float ratio);
        RectangleNode(Core* core, const Data& config);
        ~RectangleNode() override;

        const Point2D& GetPos() override;
        const Size2D& GetSize() override;

    private:

        void updateMetrics() override;

        Point2D _pos;
        Size2D _size;

        float _ratio;
    };

}

#endif /* rectangle_node_h */
