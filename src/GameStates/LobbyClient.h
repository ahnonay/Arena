#pragma once

#include <queue>
#include <SFML/Network.hpp>
#include "GameState.h"
#include "../GameObjects/Character.h"

struct LobbyClientStartData {
    std::string playerName;
    std::string hostIP;
    std::unique_ptr<sf::TcpSocket> socket;
    CHARACTERS characterType;
};

/***
 * Client version of the lobby. At the start, sends the player's name to the server.
 * Then, listens to the server to get updated player lists or the start signal (which includes the random seed).
 * Next state is either GameClient (then, the server's socket etc. is passed on as a
 * GameClientStartData object returned by LobbyClient::end()) or MainMenu.
 */
class LobbyClient : public GameState {
public:
    GAME_STATES run() override;

    void start(std::shared_ptr<void> data) override;

    std::shared_ptr<void> end() override;

private:
    std::vector<std::pair<std::string, CHARACTERS>> playersList;

    // The local player's name and type
    std::string playerName;
    CHARACTERS characterType;

    GameState::GAME_STATES nextState;
    std::string hostIP;
    std::uint32_t randomSeed;
    std::unique_ptr<sf::TcpSocket> socket;
    sf::Packet packet;
};