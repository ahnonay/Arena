#pragma once

#include <queue>
#include <random>
#include <optional>
#include "GameState.h"
#include "SFML/Graphics.hpp"
#include "SFML/Graphics/RenderTexture.hpp"
#include "SFML/Network.hpp"
#include "../GameObjects/Tilemap.h"
#include "../GameObjects/CharacterContainer.h"
#include "../GameObjects/Player.h"
#include "../GameObjects/Creep.h"
#include "../GameObjects/Guard.h"
#include "../GameObjects/Ally.h"
#include "../Render/Effect.h"
#include "../GameObjects/Skills.h"
#include "../NetworkEvents/Action.h"
#include "../NetworkEvents/Event.h"
#include "../Render/MapCircleShape.h"

struct GameStartData {
    sf::Uint32 randomSeed;
    std::vector<std::pair<std::string, CHARACTERS>> playersList;
};

/***
 * The Game class contains most of the game logic.
 *
 * Every frame, the run() method is executed and, among other things, calls network(), simulate(...) and render(...)
 * run() handles user input, window resizing etc.
 * network() is implemented by the subclasses and is different for server and client.
 * simulate(...) executes the next simulation step once enough time has passed and the necessary events have been received.
 * render(...) handles rendering and UI
 */
class Game : public GameState {
public:
    GAME_STATES run() override;

    void start(std::shared_ptr<void> data) override;

    std::shared_ptr<void> end() override;

protected:
    virtual void network() = 0;
    void simulate(const sf::Time& elapsedTime);
    void render(const sf::Time& elapsedTime);

    void spawnCreep(int spawnPointIndex);

    GameState::GAME_STATES nextState;

    sf::View viewUI;
    sf::View viewWorld;
    // In addition to rendering to screen, characters are also rendered to an ID buffer. There, pixels of a character encode the character's ID. This is used for detecting the character hovered by the player's mouse.
    sf::RenderTexture characterIDBuffer;
    // Since retrieving the rendered characterIDBuffer from the GPU is very expensive, we only do it every couple of frames. For this, we need a timer.
    unsigned int characterIDBufferUpdateTimer;
    // The most recent rendered characterIDBuffer.
    sf::Image characterIDImage;

    std::shared_ptr<Tilemap> tilemap;
    std::shared_ptr<CharacterContainer> characterContainer;
    std::vector<std::shared_ptr<Player>> playerCharacters;
    // ID of the local player. Can be used as index to playerCharacters. For the server, the ID is always 0.
    unsigned int playerIndex;
    std::list<std::shared_ptr<Creep>> creeps;
    std::vector<std::shared_ptr<Guard>> guards;
    std::list<std::shared_ptr<Ally>> allies;
    // Random generator which is initialized with the seed that gets distributed over the network at the game's start
    std::mt19937 gen;

    //////////////////////////////////////
    // Some variables related to global game state. Note that more of the game state is encoded in the Player and Creep classes.
    //////////////////////////////////////
    sf::Int32 maxLives;
    sf::Int32 lives;
    // The first creep gets ID MAX_NUM_PLAYERS (i.e., all players have smaller IDs than creeps). Whenever a new creep is spawned, this counter goes up.
    sf::Uint32 newCreepIDCounter;
    // Once the game has been lost, it can no longer be won and vice versa
    enum class GAME_OUTCOME {STILL_PLAYING, WON, LOST};
    GAME_OUTCOME outcome;

    //////////////////////////////////////
    // Some variables related to rendering
    //////////////////////////////////////
    sf::Clock deltaClock;
    // targetSelectionSkillNum != 0 if the player is currently choosing the target for a skill they want to use
    unsigned int targetSelectionSkillNum;
    // Some skills have AoE, which is indicated by this shape
    std::unique_ptr<MapCircleShape> targetSelectionGuidingShape;
    // Indicates maximum range of player attacks and certain skills
    std::unique_ptr<MapCircleShape> playerAttackRangeShape;
    // Only true if the left mouse button has just been clicked in this iteration of the game loop
    bool justClickedLeft;
    bool justClickedRight;
    std::array<bool, 10> justPressedShortcut;
    // Auto-attack can be enabled and disabled with a button and shortcut 0
    bool autoAttackEnabled;
    // The current state of the WASD keys
    std::array<bool, 4> movementKeyStates;
    // hoveredCharacter != nullptr if the player is currently hovering a character with the mouse
    Character* hoveredCharacter;
    // Dead creeps no longer interact with the game logic but are kept around in this list until their death animation has completed
    std::list<std::shared_ptr<Creep>> deadCreeps;
    // Effects (like flying arrows) do not interact with the game logic and are purely cosmetic. Objects are removed from this list once the effect has completed
    std::list<std::shared_ptr<Effect>> effects;
    // Textures for skill- and shop-buttons
    sf::Texture hpPotionTexture;
    sf::Texture mpPotionTexture;
    sf::Texture tomeTexture;
    sf::Texture autoAttackTexture;
    std::array<sf::Texture, static_cast<unsigned int>(SKILLS::NUM_SKILLS)> skillButtonTextures;
    std::array<sf::Texture, static_cast<unsigned int>(SKILLS::NUM_SKILLS)> skillButtonTexturesGray;

    // If the player makes an interaction, this gets added to the localActions queue. This queue is constantly being drained by sending actions to the server.
    std::queue<std::unique_ptr<Action>> localActions;
    // Once the events for the next simulation step have been determined, the server sends them to all clients and they are collected in the eventsToSimulate list. This gets processed by Game::simulate(...)
    std::list<std::unique_ptr<Event>> eventsToSimulate; // Conceptually, should be queue; but list has efficient .splice()
    // The last simulation step that has been executed
    unsigned int simulationStep;
    // Once enough time has passed in this timer, the next simulation step may be executed (provided all necessary events for it have been received)
    unsigned int simulationTimerMS;
    // The latest simulation step for which we have received a NoMoreEvents event, i.e., this step is ready to be executed in the simulation
    unsigned int latestSimulationStepAvailable;
};
