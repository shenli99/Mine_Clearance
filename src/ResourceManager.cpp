#include <ResourceManager.hpp>
#include <filesystem>

namespace Resources {
    #include <incbin.h>
    INCBIN(FlagPng, "../icons/flag.png");
    INCBIN(Mine, "../icons/mine.png");
    INCBIN(Win, "../icons/win.png");
    INCBIN(Lose, "../icons/lose.png");
    INCBIN(Smile, "../icons/smile.png");
}

namespace Singleton {
    auto ResourceManager::getFlagTexture() -> const sf::Texture& {
            static sf::Texture flagTexture;
            static bool initialized = false;
            if (!initialized) {
                if(flagTexture.loadFromMemory(Resources::gFlagPngData, Resources::gFlagPngSize)) {
                    flagTexture.setSmooth(true);
                    initialized = true;
                    return flagTexture;
                } else {
                    throw std::runtime_error("Failed to load flag texture");
                }
            } else {
                return flagTexture;
            }
        }

    auto ResourceManager::getMineTexture() -> const sf::Texture& {
        static sf::Texture flagTexture;
        static bool initialized = false;
        if (!initialized) {
            if(flagTexture.loadFromMemory(Resources::gMineData, Resources::gMineSize)) {
                flagTexture.setSmooth(true);
                initialized = true;
                return flagTexture;
            } else {
                throw std::runtime_error("Failed to load mine texture");
            }
        } else {
            return flagTexture;
        }
    }

    auto ResourceManager::getWinTexture() -> const sf::Texture& {
        static sf::Texture flagTexture;
        static bool initialized = false;
        if (!initialized) {
            if(flagTexture.loadFromMemory(Resources::gWinData, Resources::gWinSize)) {
                flagTexture.setSmooth(true);
                initialized = true;
                return flagTexture;
            } else {
                throw std::runtime_error("Failed to load win texture");
            }
        } else {
            return flagTexture;
        }
    }

    auto ResourceManager::getLoseTexture() -> const sf::Texture& {
        static sf::Texture flagTexture;
        static bool initialized = false;
        if (!initialized) {
            if(flagTexture.loadFromMemory(Resources::gLoseData, Resources::gLoseSize)) {
                flagTexture.setSmooth(true);
                initialized = true;
                return flagTexture;
            } else {
                throw std::runtime_error("Failed to load lose texture");
            }
        } else {
            return flagTexture;
        }
    }

    auto ResourceManager::getSmileTexture() -> const sf::Texture& {
        static sf::Texture flagTexture;
        static bool initialized = false;
        if (!initialized) {
            if(flagTexture.loadFromMemory(Resources::gSmileData, Resources::gSmileSize)) {
                flagTexture.setSmooth(true);
                initialized = true;
                return flagTexture;
            } else {
                throw std::runtime_error("Failed to load smile texture");
            }
        } else {
            return flagTexture;
        }
    }

    auto ResourceManager::getFont() -> const sf::Font& {
        static sf::Font font;
        static bool initialized = false;
        if (!initialized) {
            if(std::filesystem::exists("./yahei.ttf") && font.openFromFile("./yahei.ttf")) {
                initialized = true;
                return font;
            } else {
                throw std::runtime_error("Failed to load font");
            }
        } else {
            return font;
        }
    }
}