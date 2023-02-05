//☀Rise☀
#include "Rise/node/grid_node.h"

namespace Rise {

    REGISTER_NODE(GridNode)

    GridNode::GridNode(Core* core, const Data& data)
        : Node(core), _totalCount(1) {
        auto horizontal = data["horizontal"];
        if (horizontal.isArray()) {
            float sum = 0;
            _horizontalWeights.reserve(horizontal.size());
            _totalCount *= static_cast<uint32_t>(horizontal.size());
            for (auto weightData : horizontal) {
                float weight = weightData;
                sum += weight;
                _horizontalWeights.emplace_back(weight);
            }
            for (auto& weight : _horizontalWeights) {
                weight /= sum;
            }
        }
        else {
            uint32_t count = horizontal;
            _horizontalWeights.resize(count);
            _totalCount *= count;
            for (auto& weight : _horizontalWeights) {
                weight = 1.f / count;
            }
        }

        auto vertical = data["vertical"];
        if (vertical.isArray()) {
            float sum = 0;
            _verticalWeights.reserve(vertical.size());
            _totalCount *= static_cast<uint32_t>(vertical.size());
            for (auto weightData : vertical) {
                float weight = weightData;
                sum += weight;
                _verticalWeights.emplace_back(weight);
            }
            for (auto& weight : _verticalWeights) {
                weight /= sum;
            }
        }
        else {
            uint32_t count = vertical;
            _verticalWeights.resize(count);
            _totalCount *= count;
            for (auto& weight : _verticalWeights) {
                weight = 1.f / count;
            }
        }
    }
    GridNode::GridNode(Core* core, uint32_t horizontal, uint32_t vertical)
        : Node(core), _totalCount(horizontal * vertical) {
        {
            uint32_t count = horizontal;
            _horizontalWeights.resize(count);
            for (auto& weight : _horizontalWeights) {
                weight = 1.f / count;
            }
        }
        {
            uint32_t count = vertical;
            _verticalWeights.resize(count);
            for (auto& weight : _verticalWeights) {
                weight = 1.f / count;
            }
        }
    }
    GridNode::~GridNode() {}

    void GridNode::SetHorizontalWeights(std::vector<float>&& weights) {
        _horizontalWeights = std::move(weights);
        _totalCount = static_cast<uint32_t>(_horizontalWeights.size() * _verticalWeights.size());

        auto totalWeight = 0.f;
        for (const auto& weight : _horizontalWeights) {
            totalWeight += weight;
        }
        for (auto& weight : _horizontalWeights) {
            weight /= totalWeight;
        }

        raiseDirtyMetrics();
    }
    void GridNode::SetVerticalWeights(std::vector<float>&& weights) {
        _verticalWeights = std::move(weights);
        _totalCount = static_cast<uint32_t>(_horizontalWeights.size() * _verticalWeights.size());

        auto totalWeight = 0.f;
        for (const auto& weight : _verticalWeights) {
            totalWeight += weight;
        }
        for (auto& weight : _verticalWeights) {
            weight /= totalWeight;
        }

        raiseDirtyMetrics();
    }

    void GridNode::updateMetrics() {
        _childPoses.resize(_totalCount);
        _childSizes.resize(_totalCount);

        const auto width = _horizontalWeights.size();
        const auto height = _verticalWeights.size();

        std::vector<float> offsetHWeights(width);
        std::vector<float> offsetVWeights(height);

        float offsetH = 1.f;
        float offsetV = 1.f;

        for (auto x = width - 1; x > 0; --x) {
            offsetH -= _horizontalWeights[x];
            offsetHWeights[x] = offsetH;
        }
        offsetHWeights[0] = 0;

        for (auto y = height - 1; y > 0; --y) {
            offsetV -= _verticalWeights[y];
            offsetVWeights[y] = offsetV;
        }
        offsetVWeights[0] = 0;

        auto i = 0u;
        for (auto y = 0u; y < height; ++y) {
            for (auto x = 0u; x < width; ++x, ++i) {
                _childPoses[i].x = GetPos().x + static_cast<uint32_t>(offsetHWeights[x] * GetSize().width);
                _childPoses[i].y = GetPos().y + static_cast<uint32_t>(offsetVWeights[y] * GetSize().height);

                _childSizes[i].width = static_cast<uint32_t>(_horizontalWeights[x] * GetSize().width);
                _childSizes[i].height = static_cast<uint32_t>(_verticalWeights[y] * GetSize().height);
            }
        }
    }

    const Point2D& GridNode::GetChildPos(uint32_t index) {
        checkDirtyMetrics();
        return _childPoses[index];
    }
    const Size2D& GridNode::GetChildSize(uint32_t index) {
        checkDirtyMetrics();
        return _childSizes[index];
    }

}
