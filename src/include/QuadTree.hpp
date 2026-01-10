#pragma once

#include <GameType.hpp>
#include <IDGenerator.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
#include <array>
#include <memory>
#include <unordered_map>
#include <vector>
#include <utility>

namespace QuadTree {
    using Base::Control::BoundsPtr;
    using Base::Control::ControlBase;

    class QuadTreeNode {
    private:
        using ObjectsContianer = std::unordered_map<IDCode, BoundsPtr>;

        const static int CAPACITY = 4;
        const static int MAX_CHILDREN = 4;
        // 边界
        BoundsPtr bounds;
        // 保存控件
        ObjectsContianer objects;
        // 子节点
        std::array<std::unique_ptr<QuadTreeNode>, MAX_CHILDREN> children;
        // 是否分裂
        bool isDivided = false;
        // 分裂
        auto subdivide() -> void;

    public:
        QuadTreeNode(BoundsPtr bounds) : bounds(bounds) { objects.reserve(CAPACITY); }
        QuadTreeNode(const sf::Rect<int>&& rect) : bounds(std::make_shared<const sf::Rect<int>>(std::move(rect))) {
            objects.reserve(CAPACITY);
        }
        QuadTreeNode(const sf::Rect<int>& rect) : bounds(std::make_shared<const sf::Rect<int>>(rect)) {
            objects.reserve(CAPACITY);
        }
        QuadTreeNode(const sf::Vector2i& position, const sf::Vector2i& size)
            : bounds(std::make_shared<const sf::Rect<int>>(position, size))
        {
            objects.reserve(CAPACITY);
        }

        auto insert(const ControlBase& control) -> void;

        auto query(sf::Vector2i point) -> std::optional<IDCode>;
        auto query(sf::Vector2i point, std::vector<IDCode>& found) -> void;
        auto query(const IDCode id, BoundsPtr found) -> bool;

        auto getBounds() const -> BoundsPtr { return bounds; }

        auto remove(const IDCode id) -> bool;
    };
}