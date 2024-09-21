#pragma once

#include "Game.h"

struct GameClientStartData {
    std::vector<std::pair<std::string, CHARACTERS>> playersList;
    std::string playerName;
    std::string hostIP;
    std::unique_ptr<sf::TcpSocket> socket;
    std::uint32_t randomSeed;
};

/***
 * This class implements network functionality for the game on the client side.
 * For game logic, see the super class Game.
 *
 * Inside the Game class, all player Actions are added to the Game::localActions queue.
 * This queue is constantly being emptied by sendLocalActionsToServer. Additionally, receiveEventsFromServer
 * constantly fills the Game::eventsToSimulate queue. The received events are then processed in Game::simulate.
 */
class GameClient : public Game {
public:
    void start(std::shared_ptr<void> data) override;

    std::shared_ptr<void> end() override;

private:
    void network() override;
    void sendLocalActionsToServer();
    void receiveEventsFromServer();

    std::string hostIP;
    std::unique_ptr<sf::TcpSocket> socket;
    sf::Packet sendPacket;
    bool isSending;
    sf::Packet receivePacket;
};
