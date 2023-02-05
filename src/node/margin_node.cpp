//☀Rise☀
#include "Rise/node/margin_node.h"

namespace Rise {

    REGISTER_NODE(MarginNode);

    MarginNode::MarginNode(Core* core)
        : Node(core) {}
    MarginNode::MarginNode(Core* core, const Data& data)
        : Node(core),
        _metrics(data) {}
    MarginNode::~MarginNode() {}

    void MarginNode::updateMetrics() {
        _pos = Node::GetPos() + Size2D(_metrics.left, _metrics.top);
        _size = Node::GetSize() - Size2D(_metrics.left + _metrics.right, _metrics.top + _metrics.bottom);
    }

    const Point2D& MarginNode::GetPos() {
        checkDirtyMetrics();
        return _pos;
    }
    const Size2D& MarginNode::GetSize() {
        checkDirtyMetrics();
        return _size;
    }

}
