//☀Rise☀
#include "Rise/node/fake_index_node.h"

namespace Rise {

    REGISTER_NODE(FakeIndexNode)

    FakeIndexNode::FakeIndexNode(Core* core, const Data& data)
        : Node(core) {
        _fakeIndex = data["index"].as<uint32_t>(0);
    }
    FakeIndexNode::~FakeIndexNode() {}

}
