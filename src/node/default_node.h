//☀Rise☀
#ifndef default_node_h
#define default_node_h

#include "node.h"

namespace Rise {

    class DefaultNode : public Node {
    public:

        explicit DefaultNode(Core* core, const Data& config);
        ~DefaultNode() override;

    };

}

#endif /* default_node_h */
