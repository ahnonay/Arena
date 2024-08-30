#include "LobbyClient.h"
#include "GameClient.h"
#include "../NetworkEvents/LobbyPacketTypes.h"
#include <iostream>


void LobbyClient::start(std::shared_ptr<void> data) {
    auto startData = std::static_pointer_cast<LobbyClientStartData>(data);
    this->playerName = startData->playerName;
    this->socket = std::move(startData->socket);
    this->hostIP = startData->hostIP;
    this->characterType = startData->characterType;

    // Send own name and type to server
    // Here, for convenience, we use a blocking socket (thus avoiding a repeated sending loop as done in LobbyServer)
    this->socket->setBlocking(true);
    packet.clear();
    packet << static_cast<sf::Uint8>(LobbyClientToServerPacketTypes::UpdatePlayerName);
    packet << playerName;
    packet << static_cast<sf::Uint8>(characterType);
    this->socket->send(packet);
    this->socket->setBlocking(false);

    playersList.clear();
    nextState = GAME_STATES::LobbyClient;
}

GameState::GAME_STATES LobbyClient::run() {
    sf::Event e;
    while (window->pollEvent(e)) {
        if (e.type == sf::Event::EventType::Closed)
            nextState = GAME_STATES::End;
        if (e.type == sf::Event::Resized) {
            sf::FloatRect visibleArea(0.f, 0.f, e.size.width, e.size.height);
            window->setView(sf::View(visibleArea));
        }
    }

    /***
     * Receive what the server is sending.
     * Note that we use non-blocking sockets and thus need to poll in every iteration of this loop.
     * Here, we either get the current list of players, or the game start signal
     * (which includes the random seed for this round), or the server disconnected (then, return to MainMenu).
     */
    switch (socket->receive(packet)) {
        case sf::Socket::Done:
            std::cout << "Received type packet of type ";
            sf::Uint8 type;
            packet >> type;
            std::cout << (int) type << std::endl;
            if (type == static_cast<sf::Uint8>(LobbyServerToClientPacketTypes::StartGame)) {
                packet >> randomSeed;
                nextState = GAME_STATES::GameClient;
            } else { // LobbyServerToClientPacketTypes::UpdatePlayersList
                sf::Uint8 numPlayers;
                packet >> numPlayers;
                playersList.clear();
                std::string s;
                sf::Uint8 cType;
                for (int i = 0; i < numPlayers; i++) {
                    s.clear();
                    packet >> s;
                    packet >> cType;
                    playersList.emplace_back(s, static_cast<CHARACTERS>(cType));
                }
            }
            break;
        case sf::Socket::Disconnected:
        case sf::Socket::Error:
            std::cout << "Error on socket.receive, disconnecting..." << std::endl;
            nextState = GAME_STATES::MainMenu;
            break;
        case sf::Socket::Partial:
            std::cout << "Partial on socket.receive" << std::endl;
            break;
        default:
            break;
    }

    /***
     * Display GUI and handle interactions from buttons.
     */
    window->clear();
    imgui->prepare(false, 0);
    imgui->text(250, 50, toStr("Connected to host at ", hostIP));
    imgui->text(250, 150, toStr("Your name: ", playerName));
    imgui->text(250, 250, "List of players:");
    for (int i = 0; i < playersList.size(); i++)
        imgui->text(350, 320 + 70 * i, toStr(playersList[i].first, " (", Character::characterTypeToString(playersList[i].second), ")"));
    imgui->text(250, 800, "Waiting for host to start game...");
    if (imgui->button(5, 1000, 785, "Disconnect"))
        nextState = GAME_STATES::MainMenu;
    imgui->finish();
    window->display();

    return nextState;
}

std::shared_ptr<void> LobbyClient::end() {
    if (nextState == GAME_STATES::GameClient) {
        auto returnData = std::make_shared<GameClientStartData>();
        returnData->socket = std::move(socket);
        returnData->playerName = playerName;
        returnData->hostIP = hostIP;
        returnData->playersList = std::move(playersList);
        returnData->randomSeed = randomSeed;
        return returnData;
    } else {
        socket->setBlocking(true);
        socket->disconnect();
        socket = nullptr;
        return nullptr;
    }
}