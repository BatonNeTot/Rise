//☀Rise☀
#ifndef fake_index_node_h
#define fake_index_node_h

#include "node.h"

namespace Rise {

    class FakeIndexNode : public Node {
    public:

        explicit FakeIndexNode(Core* core, const Data& config);
        ~FakeIndexNode() override;

        uint32_t index() const override { return _fakeIndex; }

    private:

        uint32_t _fakeIndex;

    };

}

#endif /* grid_node_h */
