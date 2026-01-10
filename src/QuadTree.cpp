#include <QuadTree.hpp>
#include <algorithm>

namespace QuadTree {
    auto QuadTreeNode::subdivide() -> void {
        auto position = bounds->position;
        auto size = bounds->size / 2;

        children[0] = std::make_unique<QuadTreeNode>(position, size);
        children[1] = std::make_unique<QuadTreeNode>(position + sf::Vector2i(size.x, 0), size);
        children[2] = std::make_unique<QuadTreeNode>(position + sf::Vector2i(0, size.y), size);
        children[3] = std::make_unique<QuadTreeNode>(position + size, size);

        isDivided = true;
    }

    auto QuadTreeNode::insert(const Base::Control::ControlBase& control) -> void {
        // 有公共区才插入
        if (!bounds->findIntersection(*control.getBounds())) {
            return;
        }

        if (objects.size() < CAPACITY) {
            objects[control.getCode()] = control.getBounds();
        } else {
            if (!isDivided) subdivide();

            for (auto& child : children) 
                child->insert(control);
        }
    }

    auto QuadTreeNode::query(sf::Vector2i point) -> std::optional<IDCode> {
        if (!bounds->contains(point)) {
            return std::nullopt;
        }

        for (const auto& [obj_id, obj_bounds] : objects) {
            if (obj_bounds->contains(point) && obj_id != ID::INVALID_ID) {
                return obj_id;
            }
        }

        if (isDivided) {
            for (auto& child : children) {
                auto result = child->query(point);
                if (result != std::nullopt) {
                    return result;
                }
            }
        }

        return std::nullopt;
    }

    auto QuadTreeNode::query(sf::Vector2i point, std::vector<IDCode>& found) -> void {
        if (!bounds->contains(point)) {
            return;
        }

        for (const auto& [obj_id, obj_bounds] : objects) {
            if (obj_bounds->contains(point) && obj_id != ID::INVALID_ID) {
                found.push_back(obj_id);
            }
        }

        if (isDivided) {
            for (auto& child : children) {
                child->query(point, found);
            }
        }
    }

    auto QuadTreeNode::query(const IDCode id, BoundsPtr found) -> bool {
        auto it = objects.find(id);

        if (it != objects.end()) {
            found = it->second;
            return true; 
        } else if (isDivided)
        {
            auto it = std::find_if(children.begin(), children.end(), 
                [id, &found](const auto& child) {
                    return child->query(id, found);
                }
            );

            if (it != children.end()) {
                return true;
            }
        }

        return false;
    }

    auto QuadTreeNode::remove(const IDCode id) -> bool {
        if (objects.erase(id)) {
            return true;
        } else if (isDivided) {
            for (auto& child : children) {
                if (child->remove(id)) {
                    return true;
                }
            }
        }

        return false;
    }
}