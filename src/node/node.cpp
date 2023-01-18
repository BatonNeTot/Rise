//☀Rise☀
#include "node.h"

#include "rise.h"

namespace Rise {

    const Point2D& NComponent::GetPos() {
        return _node->GetPos();
    }
    const Size2D& NComponent::GetSize() {
        return _node->GetSize();
    }

    NodeParent::~NodeParent() {
        for (auto& child : _children) {
            child->setParent(nullptr);
        }
    }

    void NodeParent::AddChild(std::shared_ptr<Node> child) {
        if (!child) {
            return;
        }

        if (child->_parent != nullptr) {
            child->_parent->RemoveChild(child.get());
        }

        child->setParent(this);
        _children.emplace_back(child);
    }

    bool NodeParent::RemoveChild(Node* child) {
        if (!child) {
            return false;
        }

        if (child->_parent != this) {
            return false;
        }

        child->setParent(nullptr);
        _children.erase(std::find_if(_children.begin(), _children.end(),
            [child](const std::shared_ptr<Node>& childPtr) { return childPtr.get() == child; }));

        return true;
    }

    void NodeParent::drawImpl(Draw::Context context) {
        for (auto& node : Children()) {
            node->drawImpl(context);
        }
    }

    void NodeParent::updateImpl() {
        for (auto& node : Children()) {
            node->updateImpl();
        }
    }

    const Point2D& NodeParent::GetChildPos(const Node* node) {
        return GetChildPos(node->index());
    }
    const Size2D& NodeParent::GetChildSize(const Node* node) {
        return GetChildSize(node->index());
    }

    std::shared_ptr<Node> NodeParent::instantiateNode(const Data& data) {
        auto node = ResourceFabric<Node>::CreateFromData(data, Instance()->Resources());
        if (node == nullptr) {
            Instance()->Logger().Error("Unrecognized node id " + data["id"].as<std::string>());
        }
        AddChild(node);
        for (auto child : data["children"]) {
            node->instantiateNode(child);
        }
        return node;
    }

    void NodeParent::updateMetricsImpl() {
        updateMetrics();
        _dirtyMetrics = false;
        onUpdateMetrics();
    }

    void NodeParent::raiseDirtyMetrics() {
        _dirtyMetrics = true;
        for (auto& child : _children) {
            child->raiseDirtyMetrics();
        }
    }

    void NodeParent::onUpdateMetrics() {
        for (auto& child : _children) {
            child->onUpdateMetrics();
        }
    }

    void Node::onUpdateMetrics() {
        for (auto& component : _components) {
            component->onUpdateMetrics();
        }
        for (auto& child : Children()) {
            child->onUpdateMetrics();
        }
    }

    bool NodeParent::onMouseClickImpl(bool leftKey, const Point2D& point) {
        for (auto& child : _children) {
            if (child->onMouseClickImpl(leftKey, point)) {
                return true;
            }
        }
        return false;
    }

    bool Node::onMouseClickImpl(bool leftKey, const Point2D& point) {
        for (auto& child : Children()) {
            if (child->onMouseClickImpl(leftKey, point)) {
                return true;
            }
        }
        for (auto& component : _components) {
            if (component->onMouseClickImpl(leftKey, point)) {
                return true;
            }
        }
        return false;
    }

    void Node::drawImpl(Draw::Context context) {
        draw(context);
        for (auto& node : Children()) {
            node->drawImpl(context);
        }
    }

    void Node::updateImpl() {
        update();
        for (auto& node : Children()) {
            node->updateImpl();
        }
    }

    void Node::addComponent(std::shared_ptr<NComponent> component) {
        component->setNode(this);
        _components.emplace_back(component);
        if (component->canDraw()) {
            _drawComponents.emplace_back(component.get());
        }
        if (component->canUpdate()) {
            _updateComponents.emplace_back(component.get());
        }
    }

    std::shared_ptr<Node> Node::instantiateNode(const Data& data) {
        auto node = ResourceFabric<Node>::CreateFromData(data, Instance()->Resources());
        if (node == nullptr) {
            Instance()->Logger().Error("Unrecognized node id " + data["id"].as<std::string>());
        }
        AddChild(node);
        for (auto componentData : data["components"]) {
            auto component = ResourceFabric<NComponent>::CreateFromData(componentData, Instance()->Resources());
            if (component == nullptr) {
                Instance()->Logger().Error("Unrecognized component id " + componentData["id"].as<std::string>());
            }
            node->addComponent(component);
        }
        for (auto child : data["children"]) {
            node->instantiateNode(child);
        }
        return node;
    }

    REGISTER_NODE(Node);
}
