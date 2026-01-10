#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Transformable.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Clock.hpp>
#include <Singleton.hpp>
#include <IDGenerator.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <cstdio>
#include <functional>
#include <memory>
#include <stdexcept>
#include <vector>
#include <typeindex>

namespace Base {
    class MessageBase {
    public:
        virtual ~MessageBase() = default;

        virtual std::type_index getTypeIndex() const = 0;
    };

    class CloseCast : public MessageBase {

    };

    namespace Control {
        using BoundsPtr = std::shared_ptr<const sf::Rect<int>>;
        class ControlBase {
        public:
            virtual BoundsPtr getBounds() const = 0;
            virtual IDCode getCode() const = 0;

            virtual ~ControlBase() = default;

            virtual void OnClicked(std::shared_ptr<const MessageBase> message) {}
        };
    }
}

namespace Game {
    struct Cells;
    class Cell: public Base::Control::ControlBase, public sf::Drawable, public sf::Transformable {
    public:
        int m_mine_count = 0;
        bool m_is_mine = false;
        enum class CellState {
            // 没有点击，暨没有初始化雷的情况
            Empty,
            // 标记为雷
            Mine,
            // 标记为旗
            Flag,
            // 默认（雷区已经初始）
            Default,
            // 已经被打开
            Uncovered,
        };

        CellState state = CellState::Empty;

        Cell(const sf::Rect<int>& rect, sf::Font& font, int x, int y, Cells& parent);
        const int getMineCount() const { return m_mine_count; }

        Base::Control::BoundsPtr getBounds() const override;

        IDCode getCode() const override;

        void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

        void reset() {
            update(CellState::Empty);
            m_mine_count = 0;
        }

        void update(CellState state);

        void updateMineText();

        void OnClicked(std::shared_ptr<const Base::MessageBase> message) override;

        bool isMine() const { return m_is_mine; }
        void setMine(bool is_mine) { m_is_mine = is_mine; }
        void addMineCount() { ++m_mine_count; }
    private:
        std::shared_ptr<const sf::Rect<int>> m_rect;
        ID m_id;
        sf::VertexArray vertices;
        int border;
        sf::Sprite m_sprite_mine;
        sf::Sprite m_sprite_flag;
        sf::Text m_mine_count_text;
        int x, y;
        Cells& parent;
    };

    struct Cells {
        int width, height, count, uncovered, flag_mine_count;
        std::vector<Cell> _cells;
        Cells() = default;
        Cells(int w, int h, sf::Rect<int> rect, int count);
        Cell& operator()(int x, int y) {
            if (x < 0 || x >= width || y < 0 || y >= height) {
                throw std::out_of_range("Cell coordinates out of range");
            }
            return _cells[y * width + x];
        }

        using iterator = std::vector<Cell>::iterator;
        using const_iterator = std::vector<Cell>::const_iterator;
        iterator begin() { return _cells.begin(); }
        iterator end() { return _cells.end(); }
        const_iterator begin() const { return _cells.begin(); }

        void reset() {
            uncovered = width * height - count;
            for(auto& cell: _cells) cell.reset();
            flag_mine_count = 0;
        }

        void reveal(int x, int y);
    };

    class CellCoord: public Base::Control::ControlBase, public sf::Drawable {
    public:
        Cells m_cells;
        std::shared_ptr<const sf::Rect<int>> m_rect;
        CellCoord(int w, int h, const sf::Rect<int>& rect, int count = 9);
        ~CellCoord() = default;

        Base::Control::BoundsPtr getBounds() const override { return m_rect; }

        IDCode getCode() const override { return m_id.getCode(); }
        
        void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

        void reset() { m_cells.reset(); }
    private: 
        ID m_id;
    };

    class GameButton: public Base::Control::ControlBase, public sf::Drawable, public sf::Transformable {
    public:
        GameButton(const sf::Rect<int>& rect, const std::string& text);
        ~GameButton() = default;

        Base::Control::BoundsPtr getBounds() const override { return m_rect; }

        IDCode getCode() const override { return m_id.getCode(); }

        void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

        void OnClicked(std::shared_ptr<const Base::MessageBase> message) override;

        std::function<void(void)> clicked_callback;

    private:
        ID m_id;
        std::shared_ptr<const sf::Rect<int>> m_rect;
        sf::Text m_text;
        sf::VertexArray m_vertices;
        bool m_is_pressed = false;
        int border;
    };

    class GameState: public sf::Drawable, public sf::Transformable {
    public:
        enum class GameStateType {
            Playing,
            GameOver,
            GameWin,
            GameStart,
        };

        GameState(sf::Rect<int> rect);
        ~GameState() = default;

        void update();

        void stateUpdate(std::shared_ptr<const Base::MessageBase> message);

        void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

        GameStateType state = GameStateType::GameStart;
    private:
        sf::Rect<int> m_rect;
        sf::VertexArray m_vertices;
        sf::Sprite& state_sprite;
        ID m_id;

        sf::Sprite win_sprite;
        sf::Sprite lose_sprite;
        sf::Sprite simle_sprite;

        sf::Text m_time_text;

        sf::Clock m_clock;
    };
}

namespace Message {
    class GameOver : public Base::MessageBase {
    public:
        std::type_index getTypeIndex() const override { return typeid(GameOver); }
    };

    class GameWin : public Base::MessageBase {
    public:
        std::type_index getTypeIndex() const override { return typeid(GameWin); }
    };

    class GameStart : public Base::MessageBase {
    public:
        std::type_index getTypeIndex() const override { return typeid(GameStart); }
    };

    class GameReset : public Base::MessageBase {
    public:
        std::type_index getTypeIndex() const override { return typeid(GameReset); }
    };

    class Quit : public Base::MessageBase {
    public:
        std::type_index getTypeIndex() const override { return typeid(Quit); }
    };
}