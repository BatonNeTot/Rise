//☀Rise☀
#ifndef node_h
#define node_h

#include "Rise/rise_object.h"
#include "Rise/draw.h"
#include "Rise/types.h"
#include "Rise/resource_manager.h"
#include "Rise/utils/data.h"

#include <vector>

namespace Rise {

    class Window;
    class Node;
    class NodeParent;
    class IRenderTarget;

    class NComponent : public RiseObject {
    public:

        NComponent(Core* core)
            : RiseObject(core) {}
        virtual ~NComponent() = default;

        const Point2D& GetPos();
        const Size2D& GetSize();

        Node* node() const {
            return _node;
        }

    private:

        friend Node;

        void setNode(Node* node) {
            _node = node;
            onNodeSetted();
        }

        virtual void onNodeSetted() {}

        virtual void onUpdateMetrics() {};

        bool onMouseClickImpl(bool leftKey, const Point2D& point) {
            if (Square2D(GetPos(), GetSize()).isInside(point) && onMouseClick(leftKey, point)) {
                return true;
            }
            return false;
        }
        virtual bool onMouseClick(bool leftKey, const Point2D& point) { return false; }

        virtual bool canDraw() const { return false; }
        virtual bool canUpdate() const { return false; }

        virtual void draw(Draw::Context context) {};
        virtual void update() {};

        Node* _node = nullptr;

    };

    class NodeParent {
    public:

        virtual ~NodeParent();

        virtual const Point2D& GetPos() = 0;
        virtual const Size2D& GetSize() = 0;

        virtual const Point2D& GetChildPos(uint32_t index) { return GetPos(); }
        virtual const Size2D& GetChildSize(uint32_t index) { return GetSize(); }

        const Point2D& GetChildPos(const Node* node);
        const Size2D& GetChildSize(const Node* node);

        void AddChild(std::shared_ptr<Node> child);
        bool RemoveChild(Node* child);

        void ClearChildren() {
            for (auto& child : _children) {
                RemoveChild(child.get());
            }
            _children.clear();
        }

        virtual std::shared_ptr<Node> instantiateNode(const Data& data);

        std::vector<std::shared_ptr<Node>>& Children() {
            return _children;
        }
        const std::vector<std::shared_ptr<Node>>& Children() const {
            return _children;
        }

    protected:

        friend IRenderTarget;
        friend Window;

        virtual void drawImpl(Draw::Context context);
        virtual void updateImpl();

        void checkDirtyMetrics() {
            if (_dirtyMetrics) {
                updateMetricsImpl();
            }
        }

        void raiseDirtyMetrics();

    private:

        virtual void onUpdateMetrics();
        virtual bool onMouseClickImpl(bool leftKey, const Point2D& point);

        void updateMetricsImpl();
        virtual void updateMetrics() {};

        bool _dirtyMetrics = true;
        std::vector<std::shared_ptr<Node>> _children;

    };

    class Node : public NodeParent, public RiseObject {
    public:

        explicit Node(Core* core)
            : RiseObject(core) {}
        Node(Core* core, const Data& settings)
            : RiseObject(core), _settings(settings) {}
        virtual ~Node() override = default;

        virtual const Point2D& GetPos() { return GetParent() ? GetParent()->GetChildPos(this) : Point2D::zero; }
        virtual const Size2D& GetSize() { return GetParent() ? GetParent()->GetChildSize(this) : Size2D::zero; }


        NodeParent* GetParent() const {
            return _parent;
        }

        virtual void onParentSetted() {}

        void addComponent(std::shared_ptr<NComponent> component);

        template <class T>
        T* getComponent() const {
            for (auto& component : _components) {
                if (auto castedComponent = dynamic_cast<T*>(component.get())) {
                    return castedComponent;
                }
            }
            return nullptr;
        }

        virtual uint32_t index() const {
            auto& siblings = GetParent()->Children();
            for (uint32_t i = 0; i < siblings.size(); ++i) {
                if (siblings[i].get() == this) {
                    return i;
                }
            }
            return 0;
        }

        virtual std::shared_ptr<Node> instantiateNode(const Data& data);

        const Data& settings() const {
            return _settings;
        }

    private:

        friend NodeParent;

        void onUpdateMetrics() final override;
        bool onMouseClickImpl(bool leftKey, const Point2D& point) final override;

        void drawImpl(Draw::Context context) final override ;
        void updateImpl() final override;

        void draw(Draw::Context context) {
            for (auto& drawComponent : _drawComponents) {
                drawComponent->draw(context);
            }
        };

        void update() {
            for (auto& updateComponent : _updateComponents) {
                updateComponent->update();
            }
        };

        void setParent(NodeParent* parent) {
            _parent = parent;
            raiseDirtyMetrics();
            onParentSetted();
        }

        NodeParent* _parent = nullptr;

        Data _settings;

        std::vector<std::shared_ptr<NComponent>> _components;
        std::vector<NComponent*> _drawComponents;
        std::vector<NComponent*> _updateComponents;
    };

#define REGISTER_NODE_BY_KEY(node, key) REGISTER_RESOURCE_BY_KEY(Node, node, key)

#define REGISTER_NODE(node) REGISTER_NODE_BY_KEY(node, #node)

#define REGISTER_NCOMPONENT_BY_KEY(ncomponent, key) REGISTER_RESOURCE_BY_KEY(NComponent, ncomponent, key)

#define REGISTER_NCOMPONENT(ncomponent) REGISTER_NCOMPONENT_BY_KEY(ncomponent, #ncomponent)

}

#endif /* node_h */
