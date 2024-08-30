#include <iostream>
#include "GameClient.h"

void GameClient::start(std::shared_ptr<void> data) {
    auto startData = std::static_pointer_cast<GameClientStartData>(data);
    this->socket = std::move(startData->socket);
    this->hostIP = startData->hostIP;

    playerIndex = std::distance(startData->playersList.begin(),
                                std::find_if(startData->playersList.begin(), startData->playersList.end(),
                                             [&startData](auto a) {return a.first == startData->playerName;}));

    nextState = GAME_STATES::GameClient;
    sendPacket.clear();
    receivePacket.clear();
    isSending = false;

    auto gameStartData = std::make_shared<GameStartData>();
    gameStartData->randomSeed = startData->randomSeed;
    gameStartData->playersList = std::move(startData->playersList);
    Game::start(gameStartData);
}

std::shared_ptr<void> GameClient::end() {
    Game::end();
    socket->setBlocking(true);
    socket->disconnect();
    socket = nullptr;
    return nullptr;
}

void GameClient::network() {
    sendLocalActionsToServer();
    receiveEventsFromServer();
}

void GameClient::sendLocalActionsToServer() {
    // If we are not sending something else right now and there are Actions to be sent,
    // prepare a packet containing the next Action.
    if (!isSending and !localActions.empty()) {
        sendPacket.clear();
        sendPacket << *localActions.front();
        localActions.pop();
        isSending = true;
    }
    // Send a packet if we currently have one prepared. Since we are using non-blocking socket,
    // the send call must be repeated with the same packet in each iteration of the game loop, until
    // the packet has been sent.
    if (isSending) {
        switch (socket->send(sendPacket)) {
            case sf::Socket::Done:
                isSending = false;
                break;
            case sf::Socket::Disconnected:
            case sf::Socket::Error:
                std::cout << "Error on send, attempting to continue..." << std::endl;
                break;
            case sf::Socket::Partial:
                std::cout << "Partial on send" << std::endl;
                break;
            default:
                break;
        }
    }
}

void GameClient::receiveEventsFromServer() {
    // Constantly receive events from the server. If the server disconnected, return to main menu.
    // On receiving an event, we either add it to the eventsToSimulate queue or update
    // latestSimulationStepAvailable so that Game::simulate knows that it can execute the next
    // step in the simulation.
    switch (socket->receive(receivePacket)) {
        case sf::Socket::Done: {
            auto e = std::make_unique<Event>();
            receivePacket >> *e;
            if (std::holds_alternative<Event::NoMoreEvents>(e->data)) {
                assert(latestSimulationStepAvailable < e->simulationStep);
                latestSimulationStepAvailable = e->simulationStep;
            }
            eventsToSimulate.emplace_back(std::move(e));
            break;
        }
        case sf::Socket::Disconnected:
        case sf::Socket::Error:
            std::cout << "Error on receive, disconnecting..." << std::endl;
            nextState = GAME_STATES::MainMenu;
            break;
        case sf::Socket::Partial:
            std::cout << "Partial on receive" << std::endl;
            break;
        default:
            break;
    }
}
