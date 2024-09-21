#include <iostream>
#include "GameServer.h"
#include "../NetworkEvents/LobbyPacketTypes.h"

void GameServer::start(std::shared_ptr<void> data) {
    this->playerIndex = 0;
    auto startData = std::static_pointer_cast<GameServerStartData>(data);
    auto gameStartData = std::make_shared<GameStartData>();
    gameStartData->randomSeed = startData->randomSeed;
    gameStartData->playersList.emplace_back(startData->playerName, startData->characterType);
    for (auto& cStartData : startData->clients) {
        gameStartData->playersList.emplace_back(cStartData.second.first, cStartData.second.second);
        auto newClient = std::make_unique<ClientRepresentation>();
        newClient->socket = std::move(cStartData.first);
        newClient->isConnected = true;
        newClient->characterIndex = this->clients.size() + 1;
        this->clients.push_back(std::move(newClient));
    }

    nextState = GAME_STATES::GameServer;
    Game::start(gameStartData);
}

std::shared_ptr<void> GameServer::end() {
    Game::end();
    for (auto const& c: clients) {
        if (c->isConnected) {
            c->socket->setBlocking(true);
            c->socket->disconnect();
            c->socket = nullptr;
        }
    }
    clients.clear();
    return nullptr;
}

void GameServer::network() {
    receiveActionsFromClients();
    processActionsToEvents();
    sendEventsToClients();
}

void GameServer::receiveActionsFromClients() {
    // Collect actions submitted by clients in receivedActions. These are later processed in processActionsToEvents.
    // If a client disconnects, we just sent a flag that we don't need to read its socket anymore. So the character
    // will continue to exist in the game but just stop doing new actions.
    for (auto& c : clients) {
        if (!c->isConnected)
            continue;
        switch (c->socket->receive(c->receivePacket)) {
            case sf::Socket::Status::Done: {
                auto a = std::make_unique<Action>();
                c->receivePacket >> *a;
                c->receivedActions.push(std::move(a));
            } break;
            case sf::Socket::Status::Disconnected:
            case sf::Socket::Status::Error:
                std::cout << "Error on receive, disconnecting player " << playerCharacters[c->characterIndex]->getName() << std::endl;
                c->socket->setBlocking(true);
                c->socket->disconnect();
                c->socket = nullptr;
                c->isConnected = false;
                break;
            case sf::Socket::Status::Partial:
                std::cout << "Partial on receive from player " << playerCharacters[c->characterIndex]->getName() << std::endl;
                break;
            default:
                break;

        }
    }
}

void GameServer::processActionQueue(std::queue<std::unique_ptr<Action>>& actions, unsigned int characterID, unsigned int newSimulationStep,
                                    std::list<std::unique_ptr<Event>>& resultingEvents) {
    // Here, we process Actions to Events for one individual player.

    // Dead players can't take actions
    if (playerCharacters[characterID]->isDead()) {
        clearQueue(actions);
        return;
    }

    while (!actions.empty()) {
        resultingEvents.push_back(std::make_unique<Event>(Event::PlayerActionEvent{characterID, *actions.front()}, newSimulationStep));
        actions.pop();
    }
}


void GameServer::processActionsToEvents() {
    // Just before the next simulation step is due on the server, prepare Events and send them to everyone
    if (latestSimulationStepAvailable <= simulationStep and simulationTimerMS >= SIMULATION_TIME_STEP_MS / 3) {
        auto newSimulationStep = latestSimulationStepAvailable + 1;
        std::list<std::unique_ptr<Event>> newEvents;
        // Create events from actions; consider receivedActions from clients but also localActions
        for (auto& c: clients) {
            if (!c->isConnected)
                continue;
            processActionQueue(c->receivedActions, c->characterIndex, newSimulationStep, newEvents);
        }
        processActionQueue(localActions, 0, newSimulationStep, newEvents);
        newEvents.push_back(std::make_unique<Event>(Event::NoMoreEvents(), newSimulationStep));

        // Fill sendPacketQueues of the clients (need to create packets from events for that)
        for (auto & newEvent : newEvents) {
            for (auto& c: clients) {
                if (!c->isConnected)
                    continue;
                auto sendPacket = std::make_unique<sf::Packet>();
                *sendPacket << *newEvent;
                c->sendPacketQueue.push(std::move(sendPacket));
            }
        }

        // Also put events into local queue (eventsToSimulate) and update latestSimulationStepAvailable
        // This is what receiveEventsFromServer does on the Client side
        eventsToSimulate.splice(eventsToSimulate.end(), newEvents);
        latestSimulationStepAvailable = newSimulationStep;
    }
}

void GameServer::sendEventsToClients() {
    // Send the queues of events to be sent to clients
    for (auto& c: clients) {
        if (!c->isConnected)
            continue;
        if (!c->sendPacketQueue.empty()) {
            switch (c->socket->send(*c->sendPacketQueue.front())) {
                case sf::Socket::Status::Done:
                    c->sendPacketQueue.pop();
                    break;
                case sf::Socket::Status::Disconnected:
                case sf::Socket::Status::Error:
                    std::cout << "Error on send to " << playerCharacters[c->characterIndex]->getName() << ", attempting to continue..." << std::endl;
                    break;
                case sf::Socket::Status::Partial:
                    std::cout << "Partial on send to " << playerCharacters[c->characterIndex]->getName() << std::endl;
                    break;
                default:
                    break;
            }
        }
    }
}