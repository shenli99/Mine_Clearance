#include <GameType.hpp>
#include <InputManager.hpp>
#include <MessageBus.hpp>
#include <ResourceManager.hpp>
#include <IDGenerator.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/String.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <cstdio>
#include <functional>
#include <memory>

const int Beginner_para[] = {9, 9, 10};
const int Intermediate_para[] = {16, 16, 40};
const int Expert_para[] = {16, 30, 99};

const auto& current_para = Intermediate_para;

void game_state_callback(std::shared_ptr<const Base::MessageBase> message);

int main()
{
    auto window = sf::RenderWindow(sf::VideoMode({current_para[0]*30u, current_para[1]*30u}), "CMake SFML Project");
    window.setFramerateLimit(144);
    auto& message_bus = Singleton::MessageBus::getInstance();
    auto& input_manager = Singleton::InputManager::getInstance();
    auto& resource_manager = Singleton::ResourceManager::getInstance();

    input_manager.init({{0, 0},  {current_para[0]*30, current_para[1]*30}});
    auto cell_coord = Game::CellCoord(
        current_para[0], current_para[1], 
        {
            {0, 0}, 
            sf::Vector2i(current_para[0] *30, current_para[1] *30)
        },
        current_para[2]
    );

    // quit message
    ID quit_id = ID::generate();
    message_bus.subscribe(quit_id.getCode(), 
    std::function<void(std::shared_ptr<const Message::Quit>)>{
        [&window](std::shared_ptr<const Message::Quit> message) {
            if (window.isOpen()) {
                window.close();
            }
        }
    });

    ID game_state_switch = ID::generate();
    message_bus.subscribe<Message::GameWin>(game_state_switch.getCode(), 
    std::function<void(std::shared_ptr<const Message::GameWin>)>{
        [](std::shared_ptr<const Message::GameWin> message) {
            game_state_callback(message);
        }
    });

    message_bus.subscribe<Message::GameOver>(game_state_switch.getCode(), 
    std::function<void(std::shared_ptr<const Message::GameOver>)>{
        [](std::shared_ptr<const Message::GameOver> message) {
            game_state_callback(message);
        }
    });

    ID game_reset = ID::generate();
    message_bus.subscribe<Message::GameReset>(game_reset.getCode(), 
    std::function<void(std::shared_ptr<const Message::GameReset>)>{
        [&cell_coord](std::shared_ptr<const Message::GameReset> message) {
            if(message->getTypeIndex() == typeid(Message::GameReset)) {
                cell_coord.reset();
            }
        }
    });

    for(auto& cell: cell_coord.m_cells) {
        input_manager.enrol(cell, typeid(Message::ClickEvent));
        message_bus.subscribe(cell.getCode(), 
        std::function<void(std::shared_ptr<const Message::ClickEvent>)>{
            [&cell](std::shared_ptr<const Message::ClickEvent> message) {
                cell.OnClicked(message);
            }
        });
    }

    while (window.isOpen())
    {
        while (const std::optional<sf::Event> event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
            
            input_manager.handle(event);
        }

        message_bus.handle();
        window.clear();
        window.draw(cell_coord);
        window.display();
    }
}

#include <windows.h>

void game_state_callback(std::shared_ptr<const Base::MessageBase> message) {
    std::printf("Game end\n");
    auto event_code = message->getTypeIndex();
    int result = 0;
    if (event_code == typeid(Message::GameWin)) {
        result = MessageBoxW(
            NULL,
            L"恭喜你获胜!\n点击YES重开一局\n点击NO来退出",
            L"胜利",
            MB_YESNO | MB_SYSTEMMODAL 
        );
    } else if (event_code == typeid(Message::GameOver)) {
        result = MessageBoxW(
            NULL,
            L"很遗憾你失败了...\n点击YES来重开一局\n点击NO来退出",
            L"失败",
            MB_YESNO | MB_SYSTEMMODAL 
        );
    }
    if (result == 6) {
        Singleton::MessageBus::getInstance().broadcast<Message::GameReset>(
            std::make_shared<Message::GameReset>()
        );
    } else if (result == 7) {
        Singleton::MessageBus::getInstance().broadcast<Message::Quit>(
            std::make_shared<Message::Quit>()
        );
    }
}