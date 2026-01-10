#pragma once

#include <GameType.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace Singleton {
    class ResourceManager : public Singleton<ResourceManager> {
    friend class Singleton<ResourceManager>;
    private:
    
    protected:
        ResourceManager() = default;
        ~ResourceManager() = default;
        ResourceManager(const ResourceManager&) = delete;
        ResourceManager& operator=(const ResourceManager&) = delete;

    public:
        auto getFlagTexture() -> const sf::Texture&;

        auto getMineTexture() -> const sf::Texture&;
        auto getSmileTexture() -> const sf::Texture&;
        auto getWinTexture() -> const sf::Texture&;
        auto getLoseTexture() -> const sf::Texture&;

        auto getFont() -> const sf::Font&;
    };
}