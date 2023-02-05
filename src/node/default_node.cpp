//☀Rise☀
#include "Rise/node/default_node.h"

namespace Rise {

    REGISTER_NODE(DefaultNode)

    DefaultNode::DefaultNode(Core* core, const Data&)
        : Node(core) {}
    DefaultNode::~DefaultNode() {}

}
