//☀Rise☀
#include "Rise/node/rectangle_node.h"

namespace Rise {

    REGISTER_NODE(RectangleNode)

    RectangleNode::RectangleNode(Core* core, float ratio)
        : Node(core), _ratio(ratio) {}
    RectangleNode::RectangleNode(Core* core, const Data& data)
        : RectangleNode(core, data["ratio"].as<float>(1.f)) {}
    RectangleNode::~RectangleNode() {}

    void RectangleNode::updateMetrics() {
        const auto& size = Node::GetSize();
        _size.height = std::min(static_cast<uint32_t>(size.width / _ratio), size.height); 
        _size.width = static_cast<uint32_t>(_size.height * _ratio);

        _pos = Node::GetPos() + (size - _size) / 2;
    }

    const Point2D& RectangleNode::GetPos() {
        checkDirtyMetrics();
        return _pos;
    }
    const Size2D& RectangleNode::GetSize() {
        checkDirtyMetrics();
        return _size;
    }

}
