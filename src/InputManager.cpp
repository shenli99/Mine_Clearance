#include <InputManager.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>
#include <algorithm>
#include <cassert>
#include <future>

static const sf::Time TIME_OUT = sf::milliseconds(120);
static const sf::Time TIME_LONG_CLICK = sf::milliseconds(300);

namespace Singleton {
    auto InputManager::init(const sf::Rect<int> bounds) -> void {
        root = std::make_unique<QuadTree::QuadTreeNode>(bounds);
    }

    auto InputManager::enrol(Base::Control::ControlBase& control, std::type_index event_type) -> void {
        auto control_ids = controls.find(event_type);
        if (control_ids == controls.end()) {
            auto [it, success] = controls.insert({event_type, std::vector<IDCode>{control.getCode()}});
            assert(success);
            assert(controls.size() > 0);
            control_ids = it;
        } else {
            control_ids->second.emplace_back(control.getCode());
        }
        
        std::sort(control_ids->second.begin(), control_ids->second.end());
        if (root == nullptr) {
            root = std::make_unique<QuadTree::QuadTreeNode>(control.getBounds());
        }
        root->insert(control);
    }

    auto InputManager::cancel(const IDCode id) -> void {
        for(auto& [index, ids] : controls) {
            auto it = std::lower_bound(ids.begin(), ids.end(), id);
            if (it != ids.end() && *it == id) {
                ids.erase(it);
                std::sort(ids.begin(), ids.end());
            }
        }

        root->remove(id);
    }

    auto InputManager::cancel(const IDCode id, std::type_index event_type) -> void {
        auto it = std::find(controls[event_type].begin(), controls[event_type].end(), id);
        if (it != controls[event_type].end()) {
            controls[event_type].erase(it);
        }
    }

    struct MoueClickInfo {
        IDCode id;
        sf::Vector2i position;
        std::unique_ptr<sf::Clock> timer;
        int count;
    };

    auto InputManager::handle(const std::optional<sf::Event>& optional_event) -> void {
        if (optional_event.has_value()) {
            auto& event = optional_event.value();
            std::vector<std::future<void>> futures;
            // 退出事件
            if (event.is<sf::Event::Closed>()) {
                
            }
            // 处理鼠标事件
            {
                using MouseClinckInfoTable = std::array<std::optional<MoueClickInfo>, sf::Mouse::ButtonCount>;
                static MouseClinckInfoTable mouse_click_time_table = {};
                
                for (unsigned int i =0; i < sf::Mouse::ButtonCount; ++i) {
                    auto& info = mouse_click_time_table.at(i);
                    if (info.has_value()) {
                        // 单击
                        if (info->count == 1) {
                            if (info->timer->getElapsedTime() > TIME_OUT) {
                                IDCode id = info->id;
                                sf::Vector2i position = info->position;
                                info = std::nullopt;
                                auto& control_ids = controls[typeid(Message::ClickEvent)];
                                auto it = std::lower_bound(control_ids.begin(), control_ids.end(), id);
                                if (it != control_ids.end() && *it == id) {
                                    std::future<void> future = std::async(std::launch::async, [id, position, i]() {
                                        MessageBus::getInstance().send(
                                            id, 
                                            std::make_shared<Message::ClickEvent>(
                                                position,
                                                static_cast<sf::Mouse::Button>(i)
                                            )
                                        );
                                    });

                                    futures.push_back(std::move(future));
                                }
                            }
                        }
                    }
                }

                if (const auto* e = event.getIf<sf::Event::MouseButtonReleased>()) {
                    auto& info = mouse_click_time_table.at(static_cast<int>(e->button));
                    if (info.has_value()) {
                        info->timer->stop();
                        // 长按
                        if (info->timer->getElapsedTime() > TIME_LONG_CLICK) {
                            IDCode id = info->id;
                            sf::Vector2i position = info->position;
                            info = std::nullopt;
                            auto& control_ids = controls[typeid(Message::LClickEvent)];
                            auto it = std::lower_bound(control_ids.begin(), control_ids.end(), id);
                            if (it != control_ids.end() && *it == id) {
                                auto button = e->button;
                                std::future<void> future = std::async(std::launch::async, [id, position, button]() {
                                    MessageBus::getInstance().send(
                                        id, 
                                        std::make_shared<Message::LClickEvent>(position, button)
                                    );
                                });

                                futures.push_back(std::move(future));
                            }
                        }
                        // 第一次点击
                        else if (info->count == 0) {
                            info->count++;
                            info->timer->restart();
                        }
                        // 双击
                        else if (info->count == 1) {
                            IDCode id = info->id;
                            sf::Vector2i position = info->position;
                            info = std::nullopt;
                            auto& control_ids = controls[typeid(Message::DClickEvent)];
                            auto it = std::lower_bound(control_ids.begin(), control_ids.end(), id);
                            if (it != control_ids.end() && *it == id) {
                                auto button = e->button;
                                std::future<void> future = std::async(std::launch::async, [id, position, button]() {
                                    MessageBus::getInstance().send(
                                        id, 
                                        std::make_shared<Message::DClickEvent>(position, button)
                                    );
                                });

                                futures.push_back(std::move(future));
                            }
                        }
                    }
                }
                else if (const auto* e = event.getIf<sf::Event::MouseButtonPressed>()) {
                    auto& info = mouse_click_time_table.at(static_cast<int>(e->button));
                    if (info.has_value()) {
                        if (info->position == e->position) {
                            info->timer->restart();
                        } else {
                            auto new_id = root->query(e->position);
                            if (!new_id.has_value()) {
                                info = std::nullopt;
                            } else if (new_id.value() == info->id) {
                                info->timer->restart();
                            } else {
                                info->id = new_id.value();
                                info->position = e->position;
                                info->count = 0;
                                info->timer->restart();
                            }
                        }
                    } else {
                        auto new_id = root->query(e->position);
                        if (new_id.has_value()) {
                            info = MoueClickInfo{
                                new_id.value(), 
                                e->position, 
                                std::make_unique<sf::Clock>(), 
                                0
                            };
                            info->timer->restart();
                        }
                    }
                }
            }

            for (auto& future : futures) {
                future.wait();
            }
        }
    }
}