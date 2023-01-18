//☀Rise☀
#ifndef margin_node_h
#define margin_node_h

#include "node.h"

#include "image.h"

namespace Rise {

    class MarginNode : public Node {
    public:

        explicit MarginNode(Core* core);
        MarginNode(Core* core, const Data& config);
        ~MarginNode() override;

        const Point2D& GetPos() override;
        const Size2D& GetSize() override;

        void setMargin(const Padding2D& metrics) {
            _metrics = metrics;
        }

    private:

        void updateMetrics() override;

        Padding2D _metrics;

        Point2D _pos;
        Size2D _size;

    };

}

#endif /* margin_node_h */
