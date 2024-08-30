#pragma once

#include "GameState.h"
#include "../Render/IMGUI.h"
#include <iostream>
#include <list>
#include "SFML/Graphics.hpp"
#include "SFML/Network.hpp"
#include "../GameObjects/Character.h"

/**
 * Initial screen of the game. Allows users to start hosting or join an existing game.
 * Next states are then either LobbyClient or LobbyServer. MainMenu::end() returns either
 * a pointer to LobbyClientStartData or LobbyServerStartData.
 */
class MainMenu : public GameState {
public:
    MainMenu();

    void start(std::shared_ptr<void> data) override;

    GAME_STATES run() override;

    std::shared_ptr<void> end() override;

private:
    bool nameOK();

    GameState::GAME_STATES nextState;

    std::string hostIP;
    std::string playerName;
    CHARACTERS characterType;

    std::unique_ptr<sf::TcpListener> listener;
    std::unique_ptr<sf::TcpSocket> clientSocket;
};
