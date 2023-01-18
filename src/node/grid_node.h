//☀Rise☀
#ifndef grid_node_h
#define grid_node_h

#include "node.h"

namespace Rise {

    class GridNode : public Node {
    public:

        GridNode(Core* core, const Data& config);
        GridNode(Core* core, uint32_t horizontal, uint32_t vertical);
        ~GridNode() override;

        const Point2D& GetChildPos(uint32_t index) override;
        const Size2D& GetChildSize(uint32_t index) override;

        void SetHorizontalWeights(std::vector<float>&& weights);
        void SetVerticalWeights(std::vector<float>&& weights);

    private:

        void updateMetrics() override;

        std::vector<Point2D> _childPoses;
        std::vector<Size2D> _childSizes;

        std::vector<float> _horizontalWeights;
        std::vector<float> _verticalWeights;

        uint32_t _totalCount;

    };

}

#endif /* grid_node_h */
