#include "include/GameType.hpp"
#include "IDGenerator.hpp"
#include "InputManager.hpp"
#include "MessageBus.hpp"
#include <GameType.hpp>
#include <ResourceManager.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Mouse.hpp>
#include <memory>
#include <random>
#include <sstream>
#include <vector>

static void yeild_mines(Game::Cells& cells, int w = 9, int h = 9, int count = 10, int x = 0, int y = 0);

namespace Game {

    void Cells::reveal(int px, int py) {
        auto queue = std::vector<std::pair<int, int>>({{px, py}});
        while (!queue.empty()) {
            auto [x, y] = queue.back();
            queue.pop_back();
            if (x < 0 || x >= width || y < 0 || y >= height) {
                continue;
            }
            auto& cell = _cells[y * width + x];
            if (!cell.m_is_mine && cell.state != Cell::CellState::Uncovered) {
                cell.update(Cell::CellState::Uncovered);
                if (cell.m_mine_count > 0) {
                    continue;
                }
                if (x-1 >= 0 && y-1 >= 0) {
                    queue.push_back({x-1, y-1});
                }
                if (x >= 0 && y-1 >= 0) {
                    queue.push_back({x, y-1});
                }
                if (x+1 < width && y-1 >= 0) {
                    queue.push_back({x+1, y-1});
                }
                if (x-1 >= 0) {
                    queue.push_back({x-1, y});
                }
                if (x+1 < width) {
                    queue.push_back({x+1, y});
                }
                if (x-1 >= 0 && y+1 < height) {
                    queue.push_back({x-1, y+1});
                }
                if (y+1 < height) {
                    queue.push_back({x, y+1});
                }
                if (x+1 < width && y+1 < height) {
                    queue.push_back({x+1, y+1});
                }
            }
        }
    }

    Cells::Cells(int w, int h, sf::Rect<int> rect, int count)
        : width(w), height(h), count(count), uncovered(w*h - count), flag_mine_count(0)
    {
        sf::Font font = Singleton::ResourceManager::getInstance().getFont();

        if (w <=0 || h <= 0) {
            throw std::invalid_argument("Invalid dimensions");
        }
        _cells.reserve(w * h);

        int cell_width = rect.size.x / w;
        int cell_height = rect.size.y / h;

        sf::Vector2i origin = rect.position;
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                _cells.emplace_back(
                    sf::Rect<int>(
                        {origin.x + x * cell_width, origin.y + y * cell_height},
                        {cell_width, cell_height}
                    ),
                    font,
                    x, y,
                    *this
                );
            }
        }
    }

    static const sf::Color BColor[2][3] = {
        {
            sf::Color(240, 240, 240),
            sf::Color(128, 128, 128),
            sf::Color(192, 192, 192)
        },
        {
            sf::Color(128, 128, 128), 
            sf::Color(240, 240, 240), 
            sf::Color(160, 160, 160)
        },
    };

    Cell::Cell(const sf::Rect<int>& rect, sf::Font& font, int x, int y, Cells& parent) 
        : m_rect(std::make_shared<const sf::Rect<int>>(rect)),
        vertices(sf::PrimitiveType::Triangles),
        border(2),
        m_sprite_mine(Singleton::ResourceManager::getInstance().getMineTexture()),
        m_sprite_flag(Singleton::ResourceManager::getInstance().getFlagTexture()),
        m_mine_count_text(Singleton::ResourceManager::getInstance().getFont(), L"0", 25),
        x(x), y(y),
        parent(parent)
    {
        setPosition(sf::Vector2f(m_rect->position));

        m_sprite_flag.setOrigin({m_sprite_flag.getTextureRect().size.x / 2.0f, m_sprite_flag.getTextureRect().size.y / 2.0f});
        m_sprite_mine.setOrigin({m_sprite_mine.getTextureRect().size.x / 2.0f, m_sprite_mine.getTextureRect().size.y / 2.0f});

        m_sprite_mine.setPosition(sf::Vector2f(m_rect->size / 2));
        m_sprite_flag.setPosition(sf::Vector2f(m_rect->size / 2));
        
        // 设置数字
        {
            m_mine_count_text.setPosition(sf::Vector2f(m_rect->size / 2));
        }

        int cell_width = rect.size.x;
        int cell_height = rect.size.y;
        m_sprite_mine.setScale({
            static_cast<float>(cell_width - 2*border) / (m_sprite_mine.getTextureRect().size.x * 0.55f), 
            static_cast<float>(cell_height - 2*border) / (m_sprite_mine.getTextureRect().size.y * 0.55f)
        });
        m_sprite_flag.setScale({
            static_cast<float>(cell_width - 2*border) / m_sprite_flag.getTextureRect().size.x,
            static_cast<float>(cell_height - 2*border) / m_sprite_flag.getTextureRect().size.y
        });

        // 顶点
        {    
            auto size = m_rect->size;
            // 顶点布局：
            // 0---1---2
            // |  / |  / |
            // | /  | /  |
            // 3---4---5

            vertices.append(sf::Vertex(sf::Vector2f(0, 0), BColor[0][2]));        // 0
            vertices.append(sf::Vertex(sf::Vector2f(size.x, 0), BColor[0][2]));   // 1
            vertices.append(sf::Vertex(sf::Vector2f(size.x, size.y), BColor[0][2])); // 2
            vertices.append(sf::Vertex(sf::Vector2f(0, 0), BColor[0][2]));        // 3
            vertices.append(sf::Vertex(sf::Vector2f(size.x, size.y), BColor[0][2])); // 4
            vertices.append(sf::Vertex(sf::Vector2f(0, size.y), BColor[0][2]));  // 5

            // 上边框（使用三角形带）
            // 三角形1: (0,0)-(border,border)-(size.x,0)
            // 三角形2: (border,border)-(size.x,0)-(size.x-border,border)
            vertices.append(sf::Vertex(sf::Vector2f(0, 0), BColor[0][0]));
            vertices.append(sf::Vertex(sf::Vector2f(border, border), BColor[0][0]));
            vertices.append(sf::Vertex(sf::Vector2f(size.x, 0), BColor[0][0]));
            
            vertices.append(sf::Vertex(sf::Vector2f(border, border), BColor[0][0]));
            vertices.append(sf::Vertex(sf::Vector2f(size.x, 0), BColor[0][0]));
            vertices.append(sf::Vertex(sf::Vector2f(size.x - border, border), BColor[0][0]));
            
            // 左边框
            vertices.append(sf::Vertex(sf::Vector2f(0, 0), BColor[0][0]));
            vertices.append(sf::Vertex(sf::Vector2f(0, size.y), BColor[0][0]));
            vertices.append(sf::Vertex(sf::Vector2f(border, border), BColor[0][0]));
            
            vertices.append(sf::Vertex(sf::Vector2f(0, size.y), BColor[0][0]));
            vertices.append(sf::Vertex(sf::Vector2f(border, border), BColor[0][0]));
            vertices.append(sf::Vertex(sf::Vector2f(border, size.y - border), BColor[0][0]));

            // 右边框
            vertices.append(sf::Vertex(sf::Vector2f(size.x, 0), BColor[0][1]));
            vertices.append(sf::Vertex(sf::Vector2f(size.x, size.y), BColor[0][1]));
            vertices.append(sf::Vertex(sf::Vector2f(size.x - border, border), BColor[0][1]));
            
            vertices.append(sf::Vertex(sf::Vector2f(size.x, size.y), BColor[0][1]));
            vertices.append(sf::Vertex(sf::Vector2f(size.x - border, border), BColor[0][1]));
            vertices.append(sf::Vertex(sf::Vector2f(size.x - border, size.y - border), BColor[0][1]));

            // 下边框
            vertices.append(sf::Vertex(sf::Vector2f(0, size.y), BColor[0][1]));
            vertices.append(sf::Vertex(sf::Vector2f(border, size.y - border), BColor[0][1]));
            vertices.append(sf::Vertex(sf::Vector2f(size.x, size.y), BColor[0][1]));
            
            vertices.append(sf::Vertex(sf::Vector2f(border, size.y - border), BColor[0][1]));
            vertices.append(sf::Vertex(sf::Vector2f(size.x, size.y), BColor[0][1]));
            vertices.append(sf::Vertex(sf::Vector2f(size.x - border, size.y - border), BColor[0][1]));
        }
    }

    void Cell::update(CellState state) {
        if (state == this->state){
            return;
        } else {
            this->state = state;
        }
        if (state == CellState::Empty) {
            int i;
            for (i = 0; i < 6; ++i) {
                vertices[i].color = BColor[0][2];
            }

            // 上边框
            for (i = 6; i < 18; ++i) {
                vertices[i].color = BColor[0][0];
            }

            // 下边框
            for (i = 18; i < 30; ++i) {
                vertices[i].color = BColor[0][1];
            }
        }
        if (state == CellState::Uncovered) {
            int i;
            // 面
            for (i = 0; i < 6; ++i) {
                vertices[i].color = BColor[1][2];
            }

            // 上边框
            for (i = 6; i < 18; ++i) {
                vertices[i].color = BColor[1][0];
            }

            // 下边框
            for (i = 18; i < 30; ++i) {
                vertices[i].color = BColor[1][1];
            }
            if (!m_is_mine) parent.uncovered -= 1;
            if (parent.uncovered == 0) {
                Singleton::MessageBus::getInstance().broadcast<Message::GameWin>(
                    std::make_shared<Message::GameWin>()
                );
            }
        } else if (state == CellState::Flag) {
            if (m_is_mine) parent.flag_mine_count++;
            if (parent.flag_mine_count == parent.count) {
                Singleton::MessageBus::getInstance().broadcast<Message::GameWin>(
                    std::make_shared<Message::GameWin>()
                );
            }
        }
    }

    void Cell::OnClicked(std::shared_ptr<const Base::MessageBase> message) {
        const auto event_code = message->getTypeIndex();
        if (event_code == typeid(Message::ClickEvent)) {
            auto event = std::static_pointer_cast<const Message::ClickEvent>(message);
            if (event->key == sf::Mouse::Button::Left) {
                if (state == CellState::Empty) {
                    yeild_mines(parent, parent.width, parent.height, parent.count, x, y);
                    for (auto& cell : parent._cells) {
                        if (cell.state == CellState::Empty) {
                            cell.update(CellState::Default);
                            cell.updateMineText();
                        }
                    }
                    parent.reveal(x, y);
                } else if (m_is_mine) {
                    update(CellState::Uncovered);
                    Singleton::MessageBus::getInstance().broadcast<Message::GameOver>(
                        std::make_shared<Message::GameOver>()
                    );
                } else if (m_mine_count > 0) {
                    update(CellState::Uncovered);
                } else {
                    parent.reveal(x, y);
                }
            } else if (event->key == sf::Mouse::Button::Right) {
                if (state == CellState::Default || state == CellState::Empty) {
                    update(CellState::Flag);
                } else if (state == CellState::Flag) {
                    update(CellState::Default);
                }
            }
        }
    }

    Base::Control::BoundsPtr Cell::getBounds() const {
        return m_rect;
    }

    void Cell::updateMineText() {
        m_mine_count_text.setString(std::to_string(m_mine_count));
        const auto _bounds = m_mine_count_text.getLocalBounds();
        m_mine_count_text.setOrigin(_bounds.getCenter());
        float scale_x = (m_rect->size.x - 2 * border) * 0.5f / _bounds.size.x;
        m_mine_count_text.setScale({scale_x, scale_x});
    }

    IDCode Cell::getCode() const {
        return m_id.getCode();
    }

    void Cell::draw(sf::RenderTarget& target, sf::RenderStates states) const {
        states.transform *= getTransform();
        target.draw(vertices, states);
        if (state == CellState::Flag) {
            target.draw(m_sprite_flag, states);
        } else if (state == CellState::Uncovered) {
            if (m_is_mine) {
                target.draw(m_sprite_mine, states);
            } else if (m_mine_count > 0) {
                target.draw(m_mine_count_text, states);
            }
        }
    }

    CellCoord::CellCoord(int w, int h, const sf::Rect<int>& rect, int count)
        : m_cells(w, h, rect, count), m_rect(std::make_shared<const sf::Rect<int>>(rect))
    {
        if (w <= 0 || h <= 0) {
            throw std::invalid_argument("Invalid dimensions");
        }
        if (rect.size.x <= 0 || rect.size.y <= 0) {
            throw std::invalid_argument("Invalid rectangle size");
        }
    }

    void CellCoord::draw(sf::RenderTarget& target, sf::RenderStates states) const {
        for(const auto& cell : m_cells._cells) {
            target.draw(cell, states);
        }
    }

    GameButton::GameButton(const sf::Rect<int>& rect, const std::string& text)
        : m_rect(std::make_shared<const sf::Rect<int>>(rect)),
        m_text(Singleton::ResourceManager::getInstance().getFont(), text, 25),
        m_vertices(sf::PrimitiveType::Triangles)
    {
        border = 3;
        {    
            auto size = m_rect->size;

            m_vertices.append(sf::Vertex(sf::Vector2f(0, 0), BColor[0][2]));        // 0
            m_vertices.append(sf::Vertex(sf::Vector2f(size.x, 0), BColor[0][2]));   // 1
            m_vertices.append(sf::Vertex(sf::Vector2f(size.x, size.y), BColor[0][2])); // 2
            m_vertices.append(sf::Vertex(sf::Vector2f(0, 0), BColor[0][2]));        // 3
            m_vertices.append(sf::Vertex(sf::Vector2f(size.x, size.y), BColor[0][2])); // 4
            m_vertices.append(sf::Vertex(sf::Vector2f(0, size.y), BColor[0][2]));  // 5

            m_vertices.append(sf::Vertex(sf::Vector2f(0, 0), BColor[0][0]));
            m_vertices.append(sf::Vertex(sf::Vector2f(border, border), BColor[0][0]));
            m_vertices.append(sf::Vertex(sf::Vector2f(size.x, 0), BColor[0][0]));
            m_vertices.append(sf::Vertex(sf::Vector2f(border, border), BColor[0][0]));
            m_vertices.append(sf::Vertex(sf::Vector2f(size.x, 0), BColor[0][0]));
            m_vertices.append(sf::Vertex(sf::Vector2f(size.x - border, border), BColor[0][0]));

            m_vertices.append(sf::Vertex(sf::Vector2f(0, 0), BColor[0][0]));
            m_vertices.append(sf::Vertex(sf::Vector2f(0, size.y), BColor[0][0]));
            m_vertices.append(sf::Vertex(sf::Vector2f(border, border), BColor[0][0]));
            m_vertices.append(sf::Vertex(sf::Vector2f(0, size.y), BColor[0][0]));
            m_vertices.append(sf::Vertex(sf::Vector2f(border, border), BColor[0][0]));
            m_vertices.append(sf::Vertex(sf::Vector2f(border, size.y - border), BColor[0][0]));

            m_vertices.append(sf::Vertex(sf::Vector2f(size.x, 0), BColor[0][1]));
            m_vertices.append(sf::Vertex(sf::Vector2f(size.x, size.y), BColor[0][1]));
            m_vertices.append(sf::Vertex(sf::Vector2f(size.x - border, border), BColor[0][1]));
            m_vertices.append(sf::Vertex(sf::Vector2f(size.x, size.y), BColor[0][1]));
            m_vertices.append(sf::Vertex(sf::Vector2f(size.x - border, border), BColor[0][1]));
            m_vertices.append(sf::Vertex(sf::Vector2f(size.x - border, size.y - border), BColor[0][1]));

            m_vertices.append(sf::Vertex(sf::Vector2f(0, size.y), BColor[0][1]));
            m_vertices.append(sf::Vertex(sf::Vector2f(border, size.y - border), BColor[0][1]));
            m_vertices.append(sf::Vertex(sf::Vector2f(size.x, size.y), BColor[0][1]));
            m_vertices.append(sf::Vertex(sf::Vector2f(border, size.y - border), BColor[0][1]));
            m_vertices.append(sf::Vertex(sf::Vector2f(size.x, size.y), BColor[0][1]));
            m_vertices.append(sf::Vertex(sf::Vector2f(size.x - border, size.y - border), BColor[0][1]));
        }

        m_text.setPosition(sf::Vector2f(m_rect->size / 2));
        m_text.setOrigin(m_text.getLocalBounds().size / 2.0f);
        m_text.setScale({m_text.getLocalBounds().size.x / (m_rect->size.x - 2*border), m_text.getLocalBounds().size.x / (m_rect->size.x - 2*border)});
        auto& msg_s = Singleton::MessageBus::getInstance();
        msg_s.subscribe<Message::ClickEvent>(getCode(), [&](std::shared_ptr<const Message::ClickEvent> message){
            this->OnClicked(message);
        });
    }

    void GameButton::OnClicked(std::shared_ptr<const Base::MessageBase> message) {
        const auto event_code = message->getTypeIndex();
        if (event_code == typeid(Message::ClickEvent)) {
            auto event = std::static_pointer_cast<const Message::ClickEvent>(message);
            if (event->key == sf::Mouse::Button::Left) {
                if (clicked_callback != nullptr) {
                    clicked_callback();
                }
            }
        }
    }

    void GameButton::draw(sf::RenderTarget& target, sf::RenderStates states) const {
        states.transform *= getTransform();
        target.draw(m_vertices, states);
        target.draw(m_text, states);
    }

    GameState::GameState(sf::Rect<int> rect)
        : m_rect(rect),
        m_vertices(sf::PrimitiveType::Triangles),
        state_sprite(simle_sprite),
        win_sprite(Singleton::ResourceManager::getInstance().getWinTexture()),
        lose_sprite(Singleton::ResourceManager::getInstance().getLoseTexture()),
        simle_sprite(Singleton::ResourceManager::getInstance().getSmileTexture()),
        m_time_text(Singleton::ResourceManager::getInstance().getFont(), "00:00", 30)
    {
        {
            auto size = m_rect.size;

            int border = 2;

            m_vertices.append(sf::Vertex(sf::Vector2f(0, 0), BColor[0][2]));        // 0
            m_vertices.append(sf::Vertex(sf::Vector2f(size.x, 0), BColor[0][2]));   // 1
            m_vertices.append(sf::Vertex(sf::Vector2f(size.x, size.y), BColor[0][2])); // 2
            m_vertices.append(sf::Vertex(sf::Vector2f(0, 0), BColor[0][2]));        // 3
            m_vertices.append(sf::Vertex(sf::Vector2f(size.x, size.y), BColor[0][2])); // 4
            m_vertices.append(sf::Vertex(sf::Vector2f(0, size.y), BColor[0][2]));  // 5

            m_vertices.append(sf::Vertex(sf::Vector2f(0, 0), BColor[0][0]));
            m_vertices.append(sf::Vertex(sf::Vector2f(border, border), BColor[0][0]));
            m_vertices.append(sf::Vertex(sf::Vector2f(size.x, 0), BColor[0][0]));
            
            m_vertices.append(sf::Vertex(sf::Vector2f(border, border), BColor[0][0]));
            m_vertices.append(sf::Vertex(sf::Vector2f(size.x, 0), BColor[0][0]));
            m_vertices.append(sf::Vertex(sf::Vector2f(size.x - border, border), BColor[0][0]));
            
            m_vertices.append(sf::Vertex(sf::Vector2f(0, 0), BColor[0][0]));
            m_vertices.append(sf::Vertex(sf::Vector2f(0, size.y), BColor[0][0]));
            m_vertices.append(sf::Vertex(sf::Vector2f(border, border), BColor[0][0]));
            
            m_vertices.append(sf::Vertex(sf::Vector2f(0, size.y), BColor[0][0]));
            m_vertices.append(sf::Vertex(sf::Vector2f(border, border), BColor[0][0]));
            m_vertices.append(sf::Vertex(sf::Vector2f(border, size.y - border), BColor[0][0]));

            m_vertices.append(sf::Vertex(sf::Vector2f(size.x, 0), BColor[0][1]));
            m_vertices.append(sf::Vertex(sf::Vector2f(size.x, size.y), BColor[0][1]));
            m_vertices.append(sf::Vertex(sf::Vector2f(size.x - border, border), BColor[0][1]));
            
            m_vertices.append(sf::Vertex(sf::Vector2f(size.x, size.y), BColor[0][1]));
            m_vertices.append(sf::Vertex(sf::Vector2f(size.x - border, border), BColor[0][1]));
            m_vertices.append(sf::Vertex(sf::Vector2f(size.x - border, size.y - border), BColor[0][1]));

            m_vertices.append(sf::Vertex(sf::Vector2f(0, size.y), BColor[0][1]));
            m_vertices.append(sf::Vertex(sf::Vector2f(border, size.y - border), BColor[0][1]));
            m_vertices.append(sf::Vertex(sf::Vector2f(size.x, size.y), BColor[0][1]));
            
            m_vertices.append(sf::Vertex(sf::Vector2f(border, size.y - border), BColor[0][1]));
            m_vertices.append(sf::Vertex(sf::Vector2f(size.x, size.y), BColor[0][1]));
            m_vertices.append(sf::Vertex(sf::Vector2f(size.x - border, size.y - border), BColor[0][1]));
        }

        m_clock.stop();
        m_clock.reset();

        m_time_text.setPosition({rect.size.x * 0.25f, rect.size.x * 0.5f});

        win_sprite.setPosition(sf::Vector2f(rect.size / 2));
        win_sprite.setOrigin(sf::Vector2f(win_sprite.getTextureRect().size));

        lose_sprite.setPosition(sf::Vector2f(rect.size / 2));
        lose_sprite.setOrigin(sf::Vector2f(lose_sprite.getTextureRect().size));

        simle_sprite.setPosition(sf::Vector2f(rect.size / 2));
        simle_sprite.setOrigin(sf::Vector2f(simle_sprite.getTextureRect().size));

        auto& msg_s = Singleton::MessageBus::getInstance();
        msg_s.subscribe<Message::GameStart>(
            m_id.getCode(), 
            [&](std::shared_ptr<const Message::GameStart> message) {
                this->stateUpdate(message);
            }
        );

        msg_s.subscribe<Message::GameOver>(
            m_id.getCode(), 
            [&](std::shared_ptr<const Message::GameOver> message) {
                this->stateUpdate(message);
            }
        );

        msg_s.subscribe<Message::GameReset>(
            m_id.getCode(), 
            [&](std::shared_ptr<const Message::GameReset> message) {
                this->stateUpdate(message);
            }
        );

        msg_s.subscribe<Message::GameWin>(
            m_id.getCode(), 
            [&](std::shared_ptr<const Message::GameWin> message) {
                this->stateUpdate(message);
            }
        );
    }

    void GameState::stateUpdate(std::shared_ptr<const Base::MessageBase> message) {
        const auto event_code = message->getTypeIndex();
        if(event_code == typeid(Message::GameOver)) {
            state = GameState::GameStateType::GameOver;
        } else if (event_code == typeid(Message::GameWin)) {
            state = GameState::GameStateType::GameWin;
        } else if (event_code == typeid(Message::GameStart)) {
            state = GameState::GameStateType::Playing;
        } else if (event_code == typeid(Message::GameReset)) {
            state = GameState::GameStateType::GameStart;
        }
    }

    void GameState::update() {
        if (state == GameState::GameStateType::Playing) {
            int total_time = static_cast<int>(m_clock.getElapsedTime().asSeconds());
            if (total_time > 1){
                int minutes = total_time / 60;
                int seconds = total_time % 60;

                std::stringstream ss;
                ss << std::setfill('0') << std::setw(2) << minutes << ":" << std::setw(2) << seconds;
                m_time_text.setString(ss.str());
                m_time_text.setOrigin(m_time_text.getLocalBounds().size / 2.0f);
            }
            state_sprite = simle_sprite;
        } else if (state == GameState::GameStateType::GameWin) {
            state_sprite = win_sprite;
        } else if (state == GameState::GameStateType::GameOver) {
            state_sprite = lose_sprite;
        }
    }

    void GameState::draw(sf::RenderTarget& target, sf::RenderStates states) const {
        states.transform *= getTransform();
        target.draw(m_vertices, states);
        target.draw(state_sprite, states);
    }
}

static void yeild_mines(Game::Cells& cells, int w, int h, int count, int px, int py) {
    if (count == 0) {
        throw std::invalid_argument("Invalid mine count");
    } else if(w*h - 9 < count) {
        throw std::invalid_argument("Too many mines");
    } else {
        auto map = std::vector<int>();
        map.reserve(w * h - 9);
        for(int y=0; y<h; ++y) {
            for(int x=0; x<w; ++x) {
                if ((x >= px -1 && x <= px+1) && (y >= py-1 && y <= py+1)) {
                    continue;
                } else {
                    map.push_back(y*w + x);
                }
            }
        }

        std::random_device rd;
        std::mt19937 gen(rd());
        
        std::shuffle(map.begin(), map.end(), gen);

        for (int i = 0; i < count; ++i) {
            cells._cells[map[i]].setMine(true);
            int x = map[i] % w;
            int y = map[i] / w;
            if (x - 1 >= 0 && y - 1 >= 0) cells._cells[(y-1)*w + x-1].addMineCount();
            if (x - 1 >= 0 && y + 1 < h) cells._cells[(y+1)*w + x-1].addMineCount();
            if (x + 1 < w && y - 1 >= 0) cells._cells[(y-1)*w + x + 1].addMineCount();
            if (x + 1 < w && y + 1 < h) cells._cells[(y+1)*w + x + 1].addMineCount();
            if (x - 1 >= 0) cells._cells[y*w + x-1].addMineCount();
            if (x + 1 < w) cells._cells[y*w + x+1].addMineCount();
            if (y - 1 >= 0) cells._cells[(y-1)*w + x].addMineCount();
            if (y + 1 < h) cells._cells[(y+1)*w + x].addMineCount();
        }
    }
}