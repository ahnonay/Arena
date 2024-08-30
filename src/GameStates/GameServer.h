#pragma once

#include <list>
#include <SFML/Network.hpp>
#include "Game.h"

struct GameServerStartData {
    std::list<std::pair<std::unique_ptr<sf::TcpSocket>, std::pair<std::string, CHARACTERS>>> clients;
    std::string playerName;
    CHARACTERS characterType;
    sf::Uint32 randomSeed;
};

/***
 * This class implements network functionality for the game on the server side.
 * For game logic, see the super class Game.
 *
 * There are individual queues of received Actions and Events to be sent for each client.
 * We constantly fill those action queues by receiving from the players in receiveActionsFromClients.
 * We also drain the event queues by sending to them in sendEventsToClients.
 * When it is time to execute the next simulation step, we create Events from all the actions in processActionsToEvents.
 *
 * The queues for the players are processed separately. So processActionQueue is called once for each player.
 */
class GameServer : public Game {
public:
    struct ClientRepresentation {
        sf::Packet receivePacket;
        std::queue<std::unique_ptr<sf::Packet>> sendPacketQueue;
        std::unique_ptr<sf::TcpSocket> socket;
        bool isConnected;
        std::queue<std::unique_ptr<Action>> receivedActions;
        unsigned int characterIndex;
    };

    void start(std::shared_ptr<void> data) override;

    std::shared_ptr<void> end() override;

private:
    void network() override;
    void receiveActionsFromClients();
    void sendEventsToClients();
    void processActionsToEvents();
    void processActionQueue(std::queue<std::unique_ptr<Action>>& actions, unsigned int characterID, unsigned int newSimulationStep, std::list<std::unique_ptr<Event>>& resultingEvents);

    std::list<std::unique_ptr<ClientRepresentation>> clients;

};
