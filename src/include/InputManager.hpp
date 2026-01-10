#pragma once

#include <GameType.hpp>
#include <MessageBus.hpp>
#include <QuadTree.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>
#include <Singleton.hpp>
#include <memory>
#include <optional>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace Message {
    template <typename T>
    class ClickEventBase: public Base::MessageBase {
    public:
        const sf::Vector2i position;
        ClickEventBase(const sf::Vector2i& pos) : position(pos) {}
        ClickEventBase(const sf::Vector2i& pos, const sf::Mouse::Button& key) : position(pos), key(key) {}
        std::type_index getTypeIndex() const override {
            return typeid(T);
        }
        sf::Mouse::Button key;
    };

    // 单击事件
    class ClickEvent: public ClickEventBase<ClickEvent> {
    public:
        ClickEvent(const sf::Vector2i& pos, const sf::Mouse::Button& key) : ClickEventBase(pos, key) {}
    };

    // 双击事件
    class DClickEvent: public ClickEventBase<DClickEvent> {
    public:
        DClickEvent(const sf::Vector2i& pos, const sf::Mouse::Button& key) : ClickEventBase(pos, key) {}
    };

    // 长按事件
    class LClickEvent: public ClickEventBase<LClickEvent> {
    public:
        LClickEvent(const sf::Vector2i& pos, const sf::Mouse::Button& key) : ClickEventBase(pos, key) {}
    };
}

namespace Singleton {
    class InputManager : public Singleton<InputManager> {
        friend class Singleton<InputManager>;
    public:
        auto init(const sf::Rect<int> bounds) -> void;
        auto enrol(Base::Control::ControlBase& control, std::type_index event_type) -> void;
        auto cancel(const IDCode id) -> void;
        auto cancel(const IDCode id, std::type_index event_type) -> void;
        auto handle(const std::optional<sf::Event>& optional_event) -> void;

    protected:
        InputManager() {
            local_clock = std::make_unique<sf::Clock>();
            local_clock->restart();
        }
        ~InputManager() = default;
        InputManager(const InputManager&) = delete;
        InputManager& operator=(const InputManager&) = delete;

    private:
        ID m_id;
        std::unique_ptr<QuadTree::QuadTreeNode> root;
        std::unordered_map<std::type_index, std::vector<IDCode>> controls;
        std::unique_ptr<sf::Clock> local_clock;
    };
}