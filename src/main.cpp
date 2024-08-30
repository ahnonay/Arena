#include "GameStates/GameState.h"
#include "GameStates/MainMenu.h"
#include "GameStates/GameServer.h"
#include "GameStates/GameClient.h"
#include "GameStates/LobbyServer.h"
#include "GameStates/LobbyClient.h"

/***
 * Entry point to the program.
 * Here, we have a state machine running the current GameState until GameState::end is returned.
 *
 * See GameStates/Game.h for the class handling most of the game logic. Also check out README.md.
 */
int main() {
    GameState::loadStaticResources();

    // TODO Better to use RAII idiom and create objects on demand, put code from start-methods into constructor
    auto **gameStates = new GameState *[GameState::End];
    gameStates[GameState::MainMenu] = new MainMenu();
    gameStates[GameState::LobbyServer] = new LobbyServer();
    gameStates[GameState::LobbyClient] = new LobbyClient();
    gameStates[GameState::GameServer] = new GameServer();
    gameStates[GameState::GameClient] = new GameClient();

    auto currentState = GameState::MainMenu;
    gameStates[currentState]->start(nullptr);
    while (true) {
        auto newState = gameStates[currentState]->run();
        if (currentState != newState) {
            auto data = gameStates[currentState]->end();
            if (newState == GameState::End)
                break;
            currentState = newState;
            gameStates[currentState]->start(data);
        }
    }

    for (int i = 0; i < GameState::End; i++)
        delete gameStates[i];
    delete[] gameStates;
    GameState::unloadStaticResources();

    return 0;
}