#pragma once

#include "SFML/Graphics.hpp"
#include "SFML/System.hpp"
#include "../Render/IMGUI.h"

class GameState {
public:
    enum GAME_STATES {
        MainMenu, LobbyServer, LobbyClient, GameServer, GameClient, End
    };

    virtual ~GameState() = default;

    virtual void start(std::shared_ptr<void> data) {}

    virtual std::shared_ptr<void> end() { return nullptr; }

    virtual GAME_STATES run() { return GAME_STATES::End; }

    static void loadStaticResources();

    static void unloadStaticResources();

protected:
    static std::shared_ptr<sf::Font> defaultFont;
    static std::unique_ptr<IMGUI> imgui;
    static std::shared_ptr<sf::RenderWindow> window;
};
