#pragma once

#include <list>
#include <queue>
#include <SFML/Network.hpp>
#include "GameState.h"
#include "../GameObjects/Character.h"

struct LobbyServerStartData {
    std::string playerName;
    std::unique_ptr<sf::TcpListener> listener;
    CHARACTERS characterType;
};

/***
 * Server version of the lobby. Listens to new connections, distributes information to
 * all clients (updated list of players and start signal once the game begins).
 * Next state is either GameServer (then, the clients' sockets etc. are passed on as a
 * GameServerStartData object returned by LobbyServer::end()) or MainMenu.
 */
class LobbyServer : public GameState {
public:
    // LobbyServer stores one ClientRepresentation construct per connected client
    struct ClientRepresentation {
        sf::Packet receivePacket;
        std::unique_ptr<sf::Packet> sendPacket;
        std::string name;
        CHARACTERS characterType;
        std::unique_ptr<sf::TcpSocket> socket;
    };

    GAME_STATES run() override;

    void start(std::shared_ptr<void> data) override;

    std::shared_ptr<void> end() override;

private:
    // When a new player joins or a player's name is changed, all connected clients are informed
    void updatePlayersList();

    // The local player's name and type
    std::string playerName;
    CHARACTERS characterType;

    sf::Uint32 randomSeed;
    GameState::GAME_STATES nextState;
    std::unique_ptr<sf::TcpListener> listener;
    std::list<std::unique_ptr<ClientRepresentation>> clients;
    std::vector<std::pair<std::string, CHARACTERS>> playersList;
};
