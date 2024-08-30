#include "MainMenu.h"
#include <random>
#include "SFML/Network.hpp"
#include "../Constants.h"
#include "LobbyServer.h"
#include "LobbyClient.h"


MainMenu::MainMenu() : nextState(GAME_STATES::MainMenu), hostIP("localhost") {
    // At each start, select a random name, which can then be changed by the user.
    std::string possibleNames[10] = {"Berta", "Helga", "Adelheid", "Agnes", "Barbara", "Trude",
                                     "Elfriede", "Ingeborg", "Gudrun", "Chantal"};
    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> dis(0, 9);
    playerName = possibleNames[dis(gen)];
    characterType = CHARACTERS::KNIGHT;
}

void MainMenu::start(std::shared_ptr<void> data) {
    nextState = GAME_STATES::MainMenu;
}

GameState::GAME_STATES MainMenu::run() {
    bool textEntered = false;
    sf::Uint32 textEnteredUnicode = 0;
    sf::Event e;
    while (window->pollEvent(e)) {
        if (e.type == sf::Event::EventType::Closed)
            nextState = GAME_STATES::End;
        if (e.type == sf::Event::TextEntered) {
            textEntered = true;
            textEnteredUnicode = e.text.unicode;
        }
        if (e.type == sf::Event::Resized) {
            sf::FloatRect visibleArea(0.f, 0.f, e.size.width, e.size.height);
            window->setView(sf::View(visibleArea));
        }
    }

    /***
     * Display GUI and handle interactions from buttons.
     */
    window->clear();
    imgui->prepare(textEntered, textEnteredUnicode);
    imgui->text(520, 100, "ARENA", 200);
    imgui->text(300, 445, "Player name:");
    imgui->textBox(20, 640, 435, &playerName, 12);

    bool isKnight = characterType == CHARACTERS::KNIGHT;
    bool isMage = characterType == CHARACTERS::MAGE;
    bool isArcher = characterType == CHARACTERS::ARCHER;
    bool isMonk = characterType == CHARACTERS::MONK;
    if (imgui->checkBox(10, 300, 550, "Knight", isKnight, 60.f) and !isKnight)
        characterType = CHARACTERS::KNIGHT;
    if (imgui->checkBox(11, 570, 550, "Mage", isMage, 60.f) and !isMage)
        characterType = CHARACTERS::MAGE;
    if (imgui->checkBox(12, 840, 550, "Archer", isArcher, 60.f) and !isArcher)
        characterType = CHARACTERS::ARCHER;
    if (imgui->checkBox(13, 1130, 550, "Monk", isMonk, 60.f) and !isMonk)
        characterType = CHARACTERS::MONK;

    if (imgui->button(1, 300, 700, "Host") and nameOK()) {
        listener = std::make_unique<sf::TcpListener>();
        listener->setBlocking(true);
        switch (listener->listen(NETWORK_PORT)) {
            case sf::Socket::Done:
                nextState = GAME_STATES::LobbyServer;
                std::cout << "Host started listening on port " << NETWORK_PORT << std::endl;
                break;
            default:
                listener = nullptr;
                std::cout << "Error on listener.listen" << std::endl;
                break;
        }
    }
    imgui->text(555, 710, "or");
    imgui->textBox(2, 660, 700, &hostIP, 15);
    if (imgui->button(3, 1140, 700, "Join") and nameOK()) {
        clientSocket = std::make_unique<sf::TcpSocket>();
        clientSocket->setBlocking(true);
        switch (clientSocket->connect(hostIP, NETWORK_PORT)) {
            case sf::Socket::Done:
                nextState = GAME_STATES::LobbyClient;
                std::cout << "Client connected to host at " << hostIP << " on port " << NETWORK_PORT
                          << std::endl;
                break;
            default:
                clientSocket = nullptr;
                std::cout << "Error on clientSocket.connect" << std::endl;
                break;
        }
    }
    imgui->finish();
    window->display();

    return nextState;
}

std::shared_ptr<void> MainMenu::end() {
    if (nextState == GAME_STATES::LobbyServer) {
        auto returnData = std::make_shared<LobbyServerStartData>();
        returnData->listener = std::move(listener);
        returnData->playerName = playerName;
        returnData->characterType = characterType;
        return returnData;
    }
    if (nextState == GAME_STATES::LobbyClient) {
        auto returnData = std::make_shared<LobbyClientStartData>();
        returnData->socket = std::move(clientSocket);
        returnData->hostIP = hostIP;
        returnData->playerName = playerName;
        returnData->characterType = characterType;
        return returnData;
    }
    return nullptr;
}

bool MainMenu::nameOK() {
    trim(playerName);
    return !playerName.empty() and playerName.length() < 12 and playerName != "Unknown";
}
