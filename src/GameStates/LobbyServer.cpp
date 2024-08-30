#include "LobbyServer.h"
#include <iostream>
#include <cassert>
#include "../NetworkEvents/LobbyPacketTypes.h"
#include "GameServer.h"

void LobbyServer::start(std::shared_ptr<void> data) {
    auto startData = std::static_pointer_cast<LobbyServerStartData>(data);
    this->playerName = startData->playerName;
    this->characterType = startData->characterType;
    this->listener = std::move(startData->listener);

    listener->setBlocking(false);
    nextState = GAME_STATES::LobbyServer;
    clients.clear();
    updatePlayersList();

    randomSeed = std::mt19937(std::random_device{}())();
}

GameState::GAME_STATES LobbyServer::run() {
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
     * Wait for new clients to connect.
     * Once a new client connected, their name and type are initialized to dummy values and replaced
     * once we get the first message from them.
     */
    bool playersListChanged = false;
    if (playersList.size() < MAX_NUM_PLAYERS) {
        // Listen to new connections
        auto newSocket = std::make_unique<sf::TcpSocket>();
        switch (listener->accept(*newSocket)) {
            case sf::Socket::Done: {
                std::cout << "New client connected" << std::endl;
                newSocket->setBlocking(false);
                auto newClientRepresentation = std::make_unique<ClientRepresentation>();
                newClientRepresentation->socket = std::move(newSocket);
                newClientRepresentation->name = "Unknown";
                newClientRepresentation->sendPacket = nullptr;
                newClientRepresentation->characterType = CHARACTERS::KNIGHT;
                clients.push_back(std::move(newClientRepresentation));
                playersListChanged = true;
                break;
            }
            case sf::Socket::NotReady:
                break;
            default:
                std::cout << "Error on listener.accept" << std::endl;
                break;
        }
        newSocket = nullptr;
    }

    /***
     * Read what the clients are sending.
     * Here, we only either get information on their name and type or learn that they disconnected.
     */
    std::list<std::list<std::unique_ptr<ClientRepresentation>>::const_iterator> disconnectedClients;
    for (auto iter = clients.cbegin(); iter != clients.end(); ++iter) {
        switch ((*iter)->socket->receive((*iter)->receivePacket)) {
            case sf::Socket::Done:
                sf::Uint8 type;
                (*iter)->receivePacket >> type;
                assert(type == static_cast<sf::Uint8>(LobbyClientToServerPacketTypes::UpdatePlayerName));
                (*iter)->receivePacket >> (*iter)->name;
                sf::Uint8 cType;
                (*iter)->receivePacket >> cType;
                (*iter)->characterType = static_cast<CHARACTERS>(cType);
                std::cout << "Received packet, got name from " << (*iter)->name << std::endl;
                playersListChanged = true;
                break;
            case sf::Socket::Disconnected:
            case sf::Socket::Error:
                std::cout << "Error on receive, disconnecting player " << (*iter)->name << std::endl;
                (*iter)->socket->setBlocking(true);
                (*iter)->socket->disconnect();
                disconnectedClients.push_back(iter);
                playersListChanged = true;
                break;
            case sf::Socket::Partial:
                std::cout << "Partial on receive from " << (*iter)->name << std::endl;
                break;
            default:
                break;
        }
    }
    for (auto iter: disconnectedClients)
        clients.erase(iter);

    /***
     * When a client connected, disconnected, or updated their name, all clients must be informed.
     * Note that we don't send the message directly here, but just prepare a corresponding packet.
     * This is because we use asynchronous, non-blocking sockets.
     */
    if (playersListChanged) {
        updatePlayersList();
        for (auto& c: clients) {
            c->sendPacket = std::make_unique<sf::Packet>();
            *c->sendPacket << static_cast<sf::Uint8>(LobbyServerToClientPacketTypes::UpdatePlayersList);
            *c->sendPacket << static_cast<sf::Uint8>(playersList.size());
            for (const auto &s: playersList)
                *c->sendPacket << s.first << static_cast<sf::Uint8>(s.second);
        }
    }

    /***
     * If there is a packet to be sent, send it to the client.
     * The sockets are non-blocking, so the send calls return immediately and must be called repeatedly
     * in this loop until all the data has been sent.
     */
    for (auto& c: clients) {
        if (c->sendPacket) {
            switch (c->socket->send(*c->sendPacket)) {
                case sf::Socket::Done:
                    std::cout << "Sent playersList to " << c->name << std::endl;
                    c->sendPacket = nullptr;
                    break;
                case sf::Socket::Disconnected:
                case sf::Socket::Error:
                    std::cout << "Error on send to " << c->name << " retrying..." << std::endl;
                    break;
                case sf::Socket::Partial:
                    std::cout << "Partial on send to " << c->name << std::endl;
                    break;
                default:
                    break;
            }
        }
    }

    /***
     * Display GUI and handle interactions from buttons.
     */
    window->clear();
    imgui->prepare(false, 0);
    imgui->text(250, 50, "Waiting for players to join...");
    imgui->text(250, 150, toStr("Your name: ", playerName));
    imgui->text(250, 250, "List of players:");
    std::vector<std::string> playersNames;
    for (int i = 0; i < playersList.size(); i++) {
        playersNames.push_back(playersList[i].first);
        imgui->text(350, 320 + 70 * i, toStr(playersList[i].first, " (", Character::characterTypeToString(playersList[i].second), ")"));
    }
    // Check that no player name appears twice and no name is "Unknown"
    std::sort(playersNames.begin(), playersNames.end());
    if (std::unique(playersNames.begin(), playersNames.end()) == playersNames.end() &&
        std::find(playersNames.begin(), playersNames.end(), "Unknown") == std::end(playersNames)) {
        if (imgui->button(5, 250, 785, "Start game")) {
            bool everythingSent = true;
            for (auto const& c: clients) {
                if (c->sendPacket)
                    everythingSent = false;
            }
            if (everythingSent)
                nextState = GAME_STATES::GameServer;
        }
    } else
        imgui->text(250, 800, "Players need unique names");
    if (imgui->button(4, 960, 785, "Stop hosting"))
        nextState = GAME_STATES::MainMenu;
    imgui->finish();
    window->display();

    return nextState;
}

std::shared_ptr<void> LobbyServer::end() {
    listener->setBlocking(true);
    listener->close();
    listener = nullptr;
    if (nextState == GAME_STATES::GameServer) {
        auto returnData = std::make_shared<GameServerStartData>();
        returnData->playerName = playerName;
        returnData->randomSeed = randomSeed;
        returnData->characterType = characterType;
        for (auto& c: clients) {
            c->socket->setBlocking(true);
            sf::Packet startPacket;
            startPacket << static_cast<sf::Uint8>(LobbyServerToClientPacketTypes::StartGame);
            startPacket << randomSeed;
            c->socket->send(startPacket);
            c->socket->setBlocking(false);
            returnData->clients.emplace_back(std::move(c->socket), std::pair<std::string, CHARACTERS>(c->name, c->characterType));
        }
        clients.clear();
        return returnData;
    } else {
        for (auto& c: clients) {
            c->socket->setBlocking(true);
            c->socket->disconnect();
        }
        clients.clear();
        return nullptr;
    }
}

void LobbyServer::updatePlayersList() {
    playersList.clear();
    playersList.emplace_back(playerName, characterType);
    for (auto const& client: clients)
        playersList.emplace_back(client->name, client->characterType);
}
