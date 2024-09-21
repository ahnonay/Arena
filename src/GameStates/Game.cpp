#include "Game.h"
#include "SFML/Network.hpp"
#include <iostream>
#include "SFML/OpenGL.hpp"
#include "../GameObjects/Items.h"
#include "../GameObjects/Skills.h"
#include "../Render/Arrow.h"
#include "../Render/MapCircleShape.h"
#include <fpm/ios.hpp>

void Game::start(std::shared_ptr<void> data) {
    auto startData = std::static_pointer_cast<GameStartData>(data);
    gen.seed(startData->randomSeed);
    std::cout << "Running simulation with seed " << startData->randomSeed << std::endl;

    tilemap = std::make_shared<Tilemap>("Data/map/map.tmx");
    characterContainer = std::make_shared<CharacterContainer>(tilemap);
    Character::loadStaticResources();
    Arrow::loadStaticResources();
    for (int i = 0; i < startData->playersList.size(); i++)
        playerCharacters.emplace_back(std::make_shared<Player>(i, startData->playersList[i].second, tilemap->getPlayerSpawnPositions()[i], tilemap, characterContainer, gen(), startData->playersList[i].first, defaultFont));

    auto curWindowSize = window->getSize();
    viewUI.setSize(sf::Vector2f(curWindowSize));
    viewWorld.setSize(sf::Vector2f(curWindowSize));
    viewUI.setCenter(sf::Vector2f(curWindowSize) / 2.f);
    viewWorld.setCenter(tilemap->mapToWorld(playerCharacters[playerIndex]->getMapPosition()));

    simulationStep = 0;
    simulationTimerMS = 0;
    latestSimulationStepAvailable = 0;
    movementKeyStates.fill(false);
    justPressedShortcut.fill(false);
    autoAttackEnabled = true;
    justClickedLeft = false;
    justClickedRight = false;

    newCreepIDCounter = MAX_NUM_PLAYERS;
    maxLives = MAX_LIVES;
    lives = maxLives;
    outcome = GAME_OUTCOME::STILL_PLAYING;

    if (!characterIDBuffer.resize(curWindowSize, {24}))
        throw std::runtime_error("Game::start: Failed to resize characterIDBuffer");
    characterIDBuffer.setView(viewWorld);
    characterIDBufferUpdateTimer = 0;

    targetSelectionSkillNum = 0;
    hoveredCharacter = nullptr;
    targetSelectionGuidingShape = std::make_unique<MapCircleShape>(tilemap, FPMNum(0));
    playerAttackRangeShape = std::make_unique<MapCircleShape>(tilemap, FPMNum(0));
    playerAttackRangeShape->setFillColor(sf::Color(150, 150, 150, 50));

    bool success = true;
    success &= hpPotionTexture.loadFromFile("Data/items/HP_potion.png");
    success &= hpPotionTexture.generateMipmap();
    success &= mpPotionTexture.loadFromFile("Data/items/MP_potion.png");
    success &= mpPotionTexture.generateMipmap();
    success &= tomeTexture.loadFromFile("Data/items/tome.png");
    success &= tomeTexture.generateMipmap();
    success &= autoAttackTexture.loadFromFile("Data/skills/auto_attack.png");
    success &= autoAttackTexture.generateMipmap();
    success &= skillButtonTextures[static_cast<unsigned int>(SKILLS::USE_DISTRACTION)].loadFromFile("Data/skills/use_distraction.png");
    success &= skillButtonTextures[static_cast<unsigned int>(SKILLS::CARNAGE)].loadFromFile("Data/skills/carnage.png");
    success &= skillButtonTextures[static_cast<unsigned int>(SKILLS::MIGHT)].loadFromFile("Data/skills/might.png");
    success &= skillButtonTextures[static_cast<unsigned int>(SKILLS::COLD_ARROW)].loadFromFile("Data/skills/cold_arrow.png");
    success &= skillButtonTextures[static_cast<unsigned int>(SKILLS::DEATHZONE_MAGE)].loadFromFile("Data/skills/death_zone_mage.png");
    success &= skillButtonTextures[static_cast<unsigned int>(SKILLS::DEATHZONE_MONK)].loadFromFile("Data/skills/death_zone_monk.png");
    success &= skillButtonTextures[static_cast<unsigned int>(SKILLS::FIREBALL)].loadFromFile("Data/skills/fireball.png");
    success &= skillButtonTextures[static_cast<unsigned int>(SKILLS::HEAL)].loadFromFile("Data/skills/heal.png");
    success &= skillButtonTextures[static_cast<unsigned int>(SKILLS::ICE_BOMB)].loadFromFile("Data/skills/ice_bomb.png");
    success &= skillButtonTextures[static_cast<unsigned int>(SKILLS::MULTI_ARROW)].loadFromFile("Data/skills/multi_arrow.png");
    success &= skillButtonTextures[static_cast<unsigned int>(SKILLS::TANK)].loadFromFile("Data/skills/tank.png");
    success &= skillButtonTextures[static_cast<unsigned int>(SKILLS::TELEPORT)].loadFromFile("Data/skills/teleport.png");
    success &= skillButtonTextures[static_cast<unsigned int>(SKILLS::CONFUSE)].loadFromFile("Data/skills/confuse.png");
    success &= skillButtonTextures[static_cast<unsigned int>(SKILLS::CREATE_SCARECROW)].loadFromFile("Data/skills/create_scarecrow.png");
    success &= skillButtonTextures[static_cast<unsigned int>(SKILLS::MASS_HEAL)].loadFromFile("Data/skills/mass_heal.png");
    success &= skillButtonTextures[static_cast<unsigned int>(SKILLS::POISON_VIAL)].loadFromFile("Data/skills/poison_vial.png");
    success &= skillButtonTextures[static_cast<unsigned int>(SKILLS::RAGE)].loadFromFile("Data/skills/rage.png");
    std::for_each(skillButtonTextures.begin(), skillButtonTextures.end(), [&success](sf::Texture &t) { success &= t.generateMipmap(); });
    // Create grayed-out versions of the skill textures for when the skill cannot be cast (e.g. not enough MP)
    for (unsigned int i = 0; i < static_cast<unsigned int>(SKILLS::NUM_SKILLS); i++) {
        auto img = skillButtonTextures[i].copyToImage();
        for (unsigned int x = 0; x < img.getSize().x; x++) {
            for (unsigned int y = 0; y < img.getSize().y; y++) {
                auto pixel = img.getPixel({x, y});
                auto avg = (static_cast<unsigned int>(pixel.r) + static_cast<unsigned int>(pixel.g) + static_cast<unsigned int>(pixel.b)) / 3;
                img.setPixel({x, y}, sf::Color(avg, avg, avg, pixel.a));
            }
        }
        success &= skillButtonTexturesGray[i].loadFromImage(img);
    }
    if (!success)
        throw std::runtime_error("Game::start: Could not load textures for skill buttons etc.");

    deltaClock.restart();
}

std::shared_ptr<void> Game::end() {
    playerCharacters.clear();
    creeps.clear();
    guards.clear();
    allies.clear();
    deadCreeps.clear();
    effects.clear();
    clearQueue(localActions);
    eventsToSimulate.clear();
    Character::unloadStaticResources();
    Arrow::unloadStaticResources();
    return nullptr;
}

GameState::GAME_STATES Game::run() {
    /***
     * This is the game's central method. Gets called by main.cpp in a loop.
     *
     * First, we collect some key states and window events.
     */
    auto elapsedTime = deltaClock.getElapsedTime();
    deltaClock.restart();

    justClickedLeft = false;
    justClickedRight = false;
    justPressedShortcut.fill(false);

    while (const std::optional event = window->pollEvent())
    {
        if (event->is<sf::Event::Closed>())
            nextState = GAME_STATES::End;
        else if (const auto* eventData = event->getIf<sf::Event::Resized>()) {
            // Adjust views etc. for resized window
            float curZoomFactor = viewWorld.getSize().x / viewUI.getSize().x;
            auto newSizef = sf::Vector2f(eventData->size);
            viewUI.setSize(newSizef);
            viewUI.setCenter(newSizef / 2.f);
            viewWorld.setSize(newSizef);
            viewWorld.zoom(curZoomFactor);
            if (!characterIDBuffer.resize(eventData->size, {24}))
                throw std::runtime_error("Game::run: Could not resize characterIDBuffer");
            characterIDBufferUpdateTimer = 0;
        }
        else if (const auto* eventData = event->getIf<sf::Event::MouseWheelScrolled>()) {
            // Zoom in and out of world
            if (eventData->wheel == sf::Mouse::Wheel::Vertical) {
                float zoomFactor = 1.f;
                if (eventData->delta > 0)
                    zoomFactor -= elapsedTime.asSeconds() * 8.f;
                else if (eventData->delta < 0)
                    zoomFactor += elapsedTime.asSeconds() * 8.f;
                if (viewWorld.getSize().x * zoomFactor >= 200 and viewWorld.getSize().x * zoomFactor <= 4000)
                    viewWorld.zoom(zoomFactor);
            }
        } else if (const auto* eventData = event->getIf<sf::Event::MouseButtonReleased>()) {
            if (eventData->button == sf::Mouse::Button::Left)
                justClickedLeft = true;
            else if (eventData->button == sf::Mouse::Button::Right)
                justClickedRight = true;
        } else if (const auto* eventData = event->getIf<sf::Event::KeyPressed>()) {
            switch (eventData->code) {
                case sf::Keyboard::Key::Num0:
                case sf::Keyboard::Key::Numpad0:
                    justPressedShortcut[0] = true;
                break;
                case sf::Keyboard::Key::Num1:
                case sf::Keyboard::Key::Numpad1:
                    justPressedShortcut[1] = true;
                break;
                case sf::Keyboard::Key::Num2:
                case sf::Keyboard::Key::Numpad2:
                    justPressedShortcut[2] = true;
                break;
                case sf::Keyboard::Key::Num3:
                case sf::Keyboard::Key::Numpad3:
                    justPressedShortcut[3] = true;
                break;
                case sf::Keyboard::Key::Num4:
                case sf::Keyboard::Key::Numpad4:
                    justPressedShortcut[4] = true;
                break;
                case sf::Keyboard::Key::Num5:
                case sf::Keyboard::Key::Numpad5:
                    justPressedShortcut[5] = true;
                break;
                case sf::Keyboard::Key::Num6:
                case sf::Keyboard::Key::Numpad6:
                    justPressedShortcut[6] = true;
                break;
                case sf::Keyboard::Key::Num7:
                case sf::Keyboard::Key::Numpad7:
                    justPressedShortcut[7] = true;
                break;
                case sf::Keyboard::Key::Num8:
                case sf::Keyboard::Key::Numpad8:
                    justPressedShortcut[8] = true;
                break;
                case sf::Keyboard::Key::Num9:
                case sf::Keyboard::Key::Numpad9:
                    justPressedShortcut[9] = true;
                break;
                default:
                    break;
            }
        }
    }

    /***
     * Before handling user input, we make sure we have the latest game state simulated.
     */
    network();
    simulate(elapsedTime);

    /***
     * Now, we process user input. Note that there is a second code section that deals with
     * GUI interactions in Game::render(...).
     */

    /***
     * If auto-attack is enabled, a new simulation step just started, and the player is not attacking something
     * nor moving in this step, start attacking something.
     */
    if (autoAttackEnabled and simulationTimerMS == 0 and playerCharacters[playerIndex]->getAttackTargetID() == playerIndex and !playerCharacters[playerIndex]->isMoving()) {
        auto nearbyCharacters = characterContainer->getCharactersAtWithTolerance(playerCharacters[playerIndex]->getMapPosition(), playerCharacters[playerIndex]->getAttackRange());
        for (const auto& c : *nearbyCharacters) {
            if (!c->isPlayerOrAlly() and playerCharacters[playerIndex]->canAttack(c->getMapPosition())) {
                localActions.emplace(std::make_unique<Action>(Action::AttackCharacterAction{c->getID()}));
                break;
            }
        }
    }

    if (window->hasFocus()) {
        auto mousePos = sf::Mouse::getPosition(*window);

        // Move camera
        // May also multiply cameraMoveAmount with * viewWorld.getSize().x / viewUI.getSize().x
        // to get less hectic movement on closer zoom levels.
        float cameraMoveAmount = elapsedTime.asMilliseconds();
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) or (0 <= mousePos.x and mousePos.x < viewUI.getSize().x *  0.01)) {
            viewWorld.move({-cameraMoveAmount, 0.f});
        } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) or (mousePos.x <= viewUI.getSize().x and mousePos.x > viewUI.getSize().x *  0.99)) {
            viewWorld.move({cameraMoveAmount, 0.f});
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) or (mousePos.y <= viewUI.getSize().y and mousePos.y > viewUI.getSize().y *  0.99)) {
            viewWorld.move({0.f, cameraMoveAmount});
        } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) or (0 <= mousePos.y and mousePos.y < viewUI.getSize().y *  0.01)) {
            viewWorld.move({0.f, -cameraMoveAmount});
        }
        auto cameraCenter = viewWorld.getCenter();
        if (cameraCenter.x < viewWorld.getSize().x / 3)
            cameraCenter.x = viewWorld.getSize().x / 3;
        if (cameraCenter.y < viewWorld.getSize().y / 3)
            cameraCenter.y = viewWorld.getSize().y / 3;
        auto worldRightMost = tilemap->mapToWorld(sf::Vector2u(tilemap->getWidth(), 0));
        auto worldLowerMost = tilemap->mapToWorld(sf::Vector2u(tilemap->getWidth(), tilemap->getHeight()));
        if (cameraCenter.x > worldRightMost.x - viewWorld.getSize().x / 3)
            cameraCenter.x = worldRightMost.x - viewWorld.getSize().x / 3;
        if (cameraCenter.y > worldLowerMost.y - viewWorld.getSize().y / 3)
            cameraCenter.y = worldLowerMost.y - viewWorld.getSize().y / 3;
        viewWorld.setCenter(cameraCenter);

        // If currently selecting a skill target and clicking right, stop selection
        if (justClickedRight or sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape))
            targetSelectionSkillNum = 0;

        // Send Action if player movement has changed
        auto curKeys = Action::MovementKeysChangedAction();
        curKeys.keyStates = {sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W),
                       sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A),
                       sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S),
                       sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)};
        if (movementKeyStates != curKeys.keyStates) {
            localActions.push(std::make_unique<Action>(curKeys));
            movementKeyStates = curKeys.keyStates;
        }

        // Read ID of currently hovered character from characterIDBuffer
        hoveredCharacter = nullptr;
        if (mousePos.x >= 0 and mousePos.y >= 0 and mousePos.x < static_cast<int>(viewUI.getSize().x) and mousePos.y < static_cast<int>(viewUI.getSize().y)) {
            if (characterIDBufferUpdateTimer == 0) {
                // This is extremely slow. Ideally, query only one pixel from the graphics card, but I'm not sure how to do that.
                // I also tried directly using the OpenGL commands in copyToImage so at least we don't need to copy the pixel vectors around
                // but this didn't work (maybe some problem with texture flipping).
                characterIDImage = characterIDBuffer.getTexture().copyToImage();
                characterIDBufferUpdateTimer = 15;
            } else
                characterIDBufferUpdateTimer -= 1;
            auto hoveredPixel = characterIDImage.getPixel(sf::Vector2u(mousePos));
            auto decodedID = (static_cast<unsigned int>(hoveredPixel.r) << 16) + (static_cast<unsigned int>(hoveredPixel.g) << 8) + static_cast<unsigned int>(hoveredPixel.b) - 1;
            if (characterContainer->isAlive(decodedID))
                hoveredCharacter = characterContainer->getCharacterByID(decodedID);
        }

        playerAttackRangeShape->setRadius(playerCharacters[playerIndex]->getAttackRange());

        // The entire following code block realizes selection of targets for both standard attacks and skills
        // It is indicated visually whether a given target is valid (e.g., color creeps red or green depending on whether they are in attack range)
        // Also, if the player clicks left and the target was valid, send a corresponding Action to the server
        if (!playerCharacters[playerIndex]->isDead()) {
            SkillInfo skillInfo{SKILL_TARGET_TYPES::SINGLE_CREEP, FPMNum(0), true};  // characteristics of standard attack
            if (targetSelectionSkillNum != 0)  // skills can have other characteristics
                skillInfo = getSkillInfo(skillSlotToSkill(playerCharacters[playerIndex]->getType(), targetSelectionSkillNum));

            switch (skillInfo.targetType) {
                case SKILL_TARGET_TYPES::SELF: {
                    if (playerCharacters[playerIndex]->canUseSkill(targetSelectionSkillNum, 0, FPMVector2()))
                        localActions.push(std::make_unique<Action>(Action::UseSelfSkillAction{{targetSelectionSkillNum}}));
                    targetSelectionSkillNum = 0;
                } break;
                case SKILL_TARGET_TYPES::SINGLE_CREEP: {
                    if (hoveredCharacter) {
                        if ((targetSelectionSkillNum == 0 and !hoveredCharacter->isPlayerOrAlly() and playerCharacters[playerIndex]->canAttack(hoveredCharacter->getMapPosition(), skillInfo.checkLineOfSight)) or (targetSelectionSkillNum != 0 and playerCharacters[playerIndex]->canUseSkill(targetSelectionSkillNum, hoveredCharacter->getID(), FPMVector2()))) {
                            hoveredCharacter->hover(sf::Color::Green);
                            if (justClickedLeft and imgui->getLastHotItem() <= 0) {
                                if (targetSelectionSkillNum == 0)
                                    localActions.push(std::make_unique<Action>(Action::AttackCharacterAction{hoveredCharacter->getID()}));
                                else
                                    localActions.push(std::make_unique<Action>(Action::UseCharacterTargetSkillAction{{targetSelectionSkillNum}, hoveredCharacter->getID()}));
                                targetSelectionSkillNum = 0;
                            }
                        } else
                            hoveredCharacter->hover(sf::Color::Red);
                    }
                } break;
                case SKILL_TARGET_TYPES::RADIUS: {
                    targetSelectionGuidingShape->setRadius(skillInfo.radius);
                    auto mousePosInMap = tilemap->worldToMap(window->mapPixelToCoords(mousePos, viewWorld));
                    if (playerCharacters[playerIndex]->canUseSkill(targetSelectionSkillNum, 0, mousePosInMap)) {
                        targetSelectionGuidingShape->setFillColor(sf::Color(0, 255, 0, 96));
                        auto nearbyCharacters = characterContainer->getCharactersAtWithTolerance(mousePosInMap, skillInfo.radius);
                        for (const auto &c: *nearbyCharacters) {
                            auto mouseToC = mousePosInMap - c->getMapPosition();
                            if (!c->isPlayerOrAlly() and getLength(mouseToC) <= skillInfo.radius)
                                c->hover(sf::Color::Green);
                        }
                        if (justClickedLeft and imgui->getLastHotItem() <= 0) {
                            localActions.push(std::make_unique<Action>(Action::UsePositionTargetSkillAction{{targetSelectionSkillNum},mousePosInMap}));
                            targetSelectionSkillNum = 0;
                        }
                    } else
                        targetSelectionGuidingShape->setFillColor(sf::Color(255, 0, 0, 96));
                } break;
                case SKILL_TARGET_TYPES::SINGLE_ALLY: {
                    if (hoveredCharacter) {
                        if (playerCharacters[playerIndex]->canUseSkill(targetSelectionSkillNum, hoveredCharacter->getID(), FPMVector2())) {
                            hoveredCharacter->hover(sf::Color::Green);
                            if (justClickedLeft and imgui->getLastHotItem() <= 0) {
                                localActions.push(std::make_unique<Action>(Action::UseCharacterTargetSkillAction{{targetSelectionSkillNum}, hoveredCharacter->getID()}));
                                targetSelectionSkillNum = 0;
                            }
                        } else
                            hoveredCharacter->hover(sf::Color::Red);
                    }
                } break;
                case SKILL_TARGET_TYPES::FREE_SPOT: {
                    targetSelectionGuidingShape->setRadius(playerCharacters[playerIndex]->getGroundRadius());
                    assert(!skillInfo.checkLineOfSight);
                    auto mousePosInMap = tilemap->worldToMap(window->mapPixelToCoords(mousePos, viewWorld));
                    bool canUseSkill = playerCharacters[playerIndex]->canUseSkill(targetSelectionSkillNum, 0, mousePosInMap);
                    if (canUseSkill)
                        targetSelectionGuidingShape->setFillColor(sf::Color(0, 255, 0, 96));
                    else
                        targetSelectionGuidingShape->setFillColor(sf::Color(255, 0, 0, 96));
                    if (justClickedLeft and canUseSkill and imgui->getLastHotItem() <= 0) {
                        localActions.push(std::make_unique<Action>(Action::UsePositionTargetSkillAction{{targetSelectionSkillNum}, mousePosInMap}));
                        targetSelectionSkillNum = 0;
                    }
                } break;
                default:
                    std::cout << "WARNING activated selection mode for a skill that needs no target selection! doing nothing..." << std::endl;
                    break;
            }
        }
    }

    /***
     * Finally, render the game and process GUI interactions.
     */
    render(elapsedTime);

    return nextState;
}

void Game::render(const sf::Time& elapsedTime) {
    /***
     * Before the actual rendering, need to update animations (is necessary even if character is not visible at
     * the moment), perform rough culling and update drawing states (if not culled).
     * Could alternatively do culling by looping over characterContainer's characterMap, but that will only be
     * faster if we really have a ton of characters on the map
     */
#define FRUSTUM_TOLERANCE 100.f
    sf::FloatRect frustum({viewWorld.getCenter().x - viewWorld.getSize().x/2.f - FRUSTUM_TOLERANCE,
                     viewWorld.getCenter().y - viewWorld.getSize().y/2.f - FRUSTUM_TOLERANCE},
                     {viewWorld.getSize().x + 2.f * FRUSTUM_TOLERANCE,
                     viewWorld.getSize().y + 2.f * FRUSTUM_TOLERANCE});
    std::list<std::shared_ptr<Character>> charactersToDraw;
    float elapsedSeconds = elapsedTime.asSeconds();
    for (auto& pc : playerCharacters) {
        auto prevAnimationStep = static_cast<int>(pc->getAnimationStep());
        if (pc->updateDrawables(frustum, elapsedSeconds, simulationTimerMS))
            charactersToDraw.emplace_back(std::dynamic_pointer_cast<Character>(pc));
        // Show new arrows everytime an Archer player is attacking and restarts the attack animation
        if (pc->getType() == CHARACTERS::ARCHER and pc->getAnimationState() == ANIMATION_STATE::ATTACK and
            prevAnimationStep != static_cast<int>(pc->getAnimationStep()) and static_cast<int>(pc->getAnimationStep()) == ARCHER_ATTACK_ANIMATION_SHOOT_STEP
            and characterContainer->isAlive(pc->getAttackTargetID()))
            effects.push_back(std::make_shared<Arrow>(tilemap, pc->getMapPosition(), characterContainer->getCharacterByID(pc->getAttackTargetID())->getMapPosition()));
    }
    for (auto& c : creeps) {
        if (c->updateDrawables(frustum, elapsedSeconds, simulationTimerMS))
            charactersToDraw.emplace_back(std::dynamic_pointer_cast<Character>(c));
    }
    for (auto& g : guards) {
        if (g->updateDrawables(frustum, elapsedSeconds, simulationTimerMS))
            charactersToDraw.emplace_back(std::dynamic_pointer_cast<Character>(g));
    }
    for (auto& a : allies) {
        if (a->updateDrawables(frustum, elapsedSeconds, simulationTimerMS))
            charactersToDraw.emplace_back(std::dynamic_pointer_cast<Character>(a));
    }
    // For dead creeps, if their death animation is over, remove them completely
    deadCreeps.remove_if([this, &elapsedSeconds, &frustum, &charactersToDraw](auto& c){
        if (c->deathAnimationComplete())
            return true;
        else {
            if (c->updateDrawables(frustum, elapsedSeconds, simulationTimerMS))
                charactersToDraw.emplace_back(std::dynamic_pointer_cast<Character>(c));
            return false;
        }
    });
    std::list<std::shared_ptr<Effect>> effectsToDraw;
    effects.remove_if([this, &elapsedSeconds, &frustum, &effectsToDraw](auto& e){
        if (e->effectHasEnded())
            return true;
        else {
            if (e->update(frustum, elapsedSeconds, simulationStep))
                effectsToDraw.emplace_back(e);
            return false;
        }
    });

    /***
     * Now we do the actual rendering, in several stages
     *   1. Draw objects inside the game world (tilemap, characters...), with depth testing. We do this twice:
     *     1a. To the screen (with all colors)
     *     1b. To the characterIDBuffer (drawing only the ID instead of the sprites' colors)
     *   2. Draw objects outside the game world (health bars etc.), without depth testing
     *   3. Draw the GUI (no depth testing and also uses a different coordinate system
     *
     *
     * Let's start with 1a: Draw world (Tilemap, characters, etc.) on screen -> depth testing enabled
     */
    window->clear();
    window->resetGLStates();
    window->setView(viewWorld);
    glClearDepth(1.f);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_ALPHA_TEST);
    // From https://www.jordansavant.com/book/graphics/sfml/sfml2_depth_buffering.md
    // In case of problems, might have to remove the following two calls here and enable them in the individual .draw functions
    applyCurrentView(*window);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (auto const& c : charactersToDraw)
        c->drawSprite(*window);
    for (auto const& e : effectsToDraw)
        e->draw(*window);
    window->draw(*tilemap);

    /***
     * 1b: Draw character IDs to buffer
     */
    if (characterIDBufferUpdateTimer == 0) {
        bool success = characterIDBuffer.setActive(true);
        characterIDBuffer.setView(viewWorld);
        characterIDBuffer.clear();
        glClear(GL_DEPTH_BUFFER_BIT);
        for (auto const& c : charactersToDraw)
            c->drawCharacterID(characterIDBuffer);
        success &= characterIDBuffer.setActive(false);
        characterIDBuffer.display();
        if (!success)
            std::cout << "Game::render: WARNING: could not set characterIDBuffer (in)active! There will be problems with character selection." << std::endl;
    }

    /***
     * 2: Draw additional elements in world -> depth testing disabled, but still in viewWorld
     */
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_ALPHA_TEST);
    window->resetGLStates();
    for (auto const& c : charactersToDraw)
        c->drawUI(*window);
    if (targetSelectionSkillNum != 0) {
        auto skillInfo = getSkillInfo(skillSlotToSkill(playerCharacters[playerIndex]->getType(), targetSelectionSkillNum));
        if (skillInfo.targetType == SKILL_TARGET_TYPES::RADIUS or skillInfo.targetType == SKILL_TARGET_TYPES::FREE_SPOT) {
            targetSelectionGuidingShape->setPosition(window->mapPixelToCoords(sf::Mouse::getPosition(*window), viewWorld));
            window->draw(*targetSelectionGuidingShape);
        }
        if (skillInfo.targetType == SKILL_TARGET_TYPES::RADIUS or skillInfo.targetType == SKILL_TARGET_TYPES::SINGLE_CREEP or skillInfo.targetType == SKILL_TARGET_TYPES::SINGLE_ALLY) {
            playerAttackRangeShape->setPosition(tilemap->mapToWorld(playerCharacters[playerIndex]->getMapPosition()));
            window->draw(*playerAttackRangeShape);
        }
    }
    for (auto const& p : playerCharacters) {
        if (p->isZoneActive()) {
            MapCircleShape shape(tilemap, p->getZoneRadius());
            shape.setPosition(tilemap->mapToWorld(p->getZonePosition()));
            shape.setFillColor(sf::Color(0, 0, 255, 96));
            window->draw(shape);
        }
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::M)) {  // Only for debugging
        for (int x = 0; x < tilemap->getWidth(); x++) {
            for (int y = 0; y < tilemap->getHeight(); y++) {
                auto tileCenter = FPMVector2(FPMNum(x + 0.5f), FPMNum(y + 0.5f));
                sf::Text textDraw(*defaultFont, std::to_string(characterContainer->getCharactersAt(tileCenter).size()), 10);
                textDraw.setPosition(tilemap->mapToWorld(tileCenter));
                textDraw.setFillColor(sf::Color::White);
                window->draw(textDraw);
            }
        }
    }

    /***
     * 3: Draw GUI elements outside world -> depth testing disabled, viewUI
     */
    window->setView(viewUI);
    imgui->prepare(false, 0);
    unsigned int toolTipFontSize = 13;
    // Tooltip with character stats
    if (hoveredCharacter)
        imgui->toolTip(toStr(Character::characterTypeToString(hoveredCharacter->getType()), "\n", "HP: ", hoveredCharacter->getHp(), " / ", hoveredCharacter->getMaxHp(), "\nMovement: ", hoveredCharacter->getMaxMovementPerSecond(),
                                  "\nRange: ", hoveredCharacter->getAttackRange(), "\nDamage: ", hoveredCharacter->getAttackDamageHP(), "\nCooldown: ", hoveredCharacter->getAttackCooldownMS()), toolTipFontSize);

    // HP, MP bars etc.
    imgui->text(10, 10, "Lives remaining:", 30);
    imgui->text(10, 50, toStr(lives, " / ", maxLives), 40);

    unsigned int conditionIconsRendered = 0;
    float conditionIconSizeX = 35;
    for (unsigned int c = 0; c < static_cast<int>(CONDITIONS::CONDITIONS_COUNT); c++) {
        if (playerCharacters[playerIndex]->hasCondition(static_cast<CONDITIONS>(c))) {
            sf::Sprite conditionSprite(*playerCharacters[playerIndex]->getConditionIcon(static_cast<CONDITIONS>(c)));
            float iconX = 10 + conditionIconsRendered * (conditionIconSizeX * 5.f / 4.f);
            float iconY = 605.f;
            imgui->transformXY(iconX, iconY);
            conditionSprite.setPosition({iconX, iconY});
            float scale = imgui->getScale() * conditionIconSizeX / conditionSprite.getTexture().getSize().x;
            conditionSprite.setScale({scale, scale});
            window->draw(conditionSprite);
            conditionIconsRendered += 1;
        }
    }

    imgui->text(10, 650, toStr(playerCharacters[playerIndex]->getName(), " (Level ", playerCharacters[playerIndex]->getLevel(), ")"), 40);

    float x = 70.f, y = 710.f, w = 200.f, h = 35.f;
    imgui->text(10, y, "HP:", 30);
    if (playerCharacters[playerIndex]->isDead())
        imgui->fillBar(x, y, (float)(playerCharacters[playerIndex]->getRespawnTimer()), (float)(playerCharacters[playerIndex]->getRespawnCooldownMS()), w, h, sf::Color::Green, toolTipFontSize);
    else
        imgui->fillBar(x, y, (float)(playerCharacters[playerIndex]->getHp()), (float)(playerCharacters[playerIndex]->getMaxHp()), w, h, sf::Color::Red, toolTipFontSize);

    y += 50.f;
    imgui->text(10, y, "MP:", 30);
    if (playerCharacters[playerIndex]->getMaxMp() > FPMNum(0))
        imgui->fillBar(x, y, (float)(playerCharacters[playerIndex]->getMp()), (float)(playerCharacters[playerIndex]->getMaxMp()), w, h, sf::Color::Blue, toolTipFontSize);

    y += 50.f;
    imgui->text(10, y, "XP:", 30);
    imgui->fillBar(x, y, (float)(playerCharacters[playerIndex]->getXp()), (float)(playerCharacters[playerIndex]->getXpForNextLevel()), w, h, sf::Color::Yellow, toolTipFontSize);

    y += 45.f;
    imgui->text(10, y, toStr(playerCharacters[playerIndex]->getGold(), " Gold"), 30);

    // Backdrop for skill bar
    sf::RectangleShape backdropSkills{sf::Vector2f{60 * imgui->getScale(), 900 * imgui->getScale()}};
    backdropSkills.setFillColor(sf::Color(200, 200, 200, 50));
    x = 1540;
    y = 0;
    imgui->transformXY(x, y);
    backdropSkills.setPosition({x, y});
    window->draw(backdropSkills);

    // Buttons for auto attack/using potions (same for all characters)
    if (imgui->button(100, 1540, 840, "", &autoAttackTexture, 60, 60, 30, autoAttackEnabled, toStr("Auto attack ", autoAttackEnabled ? "on" : "off", "\nShortcut 0"), toolTipFontSize) or justPressedShortcut[0])
        autoAttackEnabled = !autoAttackEnabled;
    if ((imgui->button(108, 1540, 700, std::to_string(playerCharacters[playerIndex]->getNumHPPotions()), &hpPotionTexture, 60, 60, 30, false, toStr("Health potions: ", playerCharacters[playerIndex]->getNumHPPotions(), "\nHeal ", ITEM_HP_POTION_HP_GAIN, " HP\nShortcut 8"), toolTipFontSize) or justPressedShortcut[8]) and playerCharacters[playerIndex]->canUsePotion(POTIONS::HP))
        localActions.push(std::make_unique<Action>(Action::UsePotionAction{POTIONS::HP}));
    if ((imgui->button(109, 1540, 770, std::to_string(playerCharacters[playerIndex]->getNumMPPotions()), &mpPotionTexture, 60, 60, 30, false, toStr("Mana potions: ", playerCharacters[playerIndex]->getNumMPPotions(), "\nRegain ", ITEM_MP_POTION_MP_GAIN, " MP\nShortcut 9"), toolTipFontSize) or justPressedShortcut[9]) and playerCharacters[playerIndex]->canUsePotion(POTIONS::MP))
        localActions.push(std::make_unique<Action>(Action::UsePotionAction{POTIONS::MP}));

    // Buttons for skills (different per character type)
    if (targetSelectionSkillNum != 0) {
        imgui->text(630, 130, "Select target!", 70);
        if (playerCharacters[playerIndex]->getMp() < getMPCostOfSkill(skillSlotToSkill(playerCharacters[playerIndex]->getType(), targetSelectionSkillNum)))
            imgui->text(650, 210, "Not enough MP...", 50);
    }
    for (unsigned int skillSlot = 1; skillSlot < 7; skillSlot++) {
        auto skill = skillSlotToSkill(playerCharacters[playerIndex]->getType(), skillSlot);
        if (skill == SKILLS::NUM_SKILLS)
            continue;
        bool isPassiveSkill = getSkillInfo(skill).targetType == SKILL_TARGET_TYPES::PASSIVE;
        bool enoughMp = isPassiveSkill || playerCharacters[playerIndex]->getMp() >= getMPCostOfSkill(skill);
        if (imgui->button(100 + skillSlot, 1540, 70 * (skillSlot - 1), "", enoughMp ? &skillButtonTextures[static_cast<int>(skill)] : &skillButtonTexturesGray[static_cast<int>(skill)], 60, 60, 30, isPassiveSkill or targetSelectionSkillNum == skillSlot, playerCharacters[playerIndex]->getSkillTooltipText(skillSlot), toolTipFontSize) or justPressedShortcut[skillSlot]) {
            if (!isPassiveSkill)
                targetSelectionSkillNum = skillSlot;
        }
        if (playerCharacters[playerIndex]->getSkillTimerSec(skillSlot) > 0.f)
            imgui->fillBar(1540, 70 * (skillSlot - 1) + 25, playerCharacters[playerIndex]->getSkillTimerSec(skillSlot), playerCharacters[playerIndex]->getSkillCooldownSec(skillSlot), 60, 10, sf::Color::Blue, toolTipFontSize);
        if (playerCharacters[playerIndex]->canUpgradeSkill(skillSlot) and imgui->button(900 + skillSlot, 1510, 70 * (skillSlot - 1) + 15, "+", nullptr, 30, 30, 30, false, getSkillLevelupTooltipText(skill), toolTipFontSize))
            localActions.push(std::make_unique<Action>(Action::UpgradeSkillAction{skillSlot}));
    }

    // Shop
    if (tilemap->getShop().contains(playerCharacters[playerIndex]->getMapPosition())) {
        sf::RectangleShape backdropShop{sf::Vector2f{240 * imgui->getScale(), 320 * imgui->getScale()}};
        backdropShop.setFillColor(sf::Color(150, 150, 150, 150));
        x = 180;
        y = 250;
        imgui->transformXY(x, y);
        backdropShop.setPosition({x, y});
        window->draw(backdropShop);

        imgui->text(250, 270, "Shop", 45);
        auto itemToBuy = ITEMS::NUM_ITEMS;
        // Inverse drawing order to ensure that tooltips are drawn correctly
        for (int i = 8; i >= 0; i--) {
            x = 200 + (i % 3) * 70;
            y = 350 + (i / 3) * 70;
            auto item = static_cast<ITEMS>(i);
            sf::Texture* img = &tomeTexture;
            if (i == 0)
                img = &hpPotionTexture;
            else if (i == 1)
                img = &mpPotionTexture;
            if (imgui->button(200 + i, x, y, getItemTitle(item), img, 60, 60, 14, false, getItemTooltip(item), toolTipFontSize) and playerCharacters[playerIndex]->canBuyItem(item))
                itemToBuy = item;
        }
        if (itemToBuy != ITEMS::NUM_ITEMS)
            localActions.push(std::make_unique<Action>(Action::BuyItemAction{itemToBuy}));
    }

    // Draw game outcomes
    if (outcome == GAME_OUTCOME::LOST)
        imgui->text(650, 400, "You lose!", 80);
    else if (outcome == GAME_OUTCOME::WON)
        imgui->text(650, 400, "You won!", 80);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::H)) {  // Only for debugging
        sf::Sprite sprite(characterIDBuffer.getTexture());
        window->draw(sprite);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F))  // Only for debugging
        imgui->text(1400, 10, toStr("Visible characters: ", charactersToDraw.size()), 20);
    imgui->finish();

    window->display();
}

void Game::simulate(const sf::Time& elapsedTime) {
    // Simulate the game for one step IFF enough time has passed since the last simulation step AND we have a new simulation step ready in eventsToSimulate
    simulationTimerMS += elapsedTime.asMilliseconds();
    bool simulationStepOverdue = simulationTimerMS > SIMULATION_TIME_STEP_MS;
    while ((latestSimulationStepAvailable > simulationStep + 1) or // we're lagging behind!
           (simulationStepOverdue and latestSimulationStepAvailable > simulationStep)) {
        simulationTimerMS = 0;
        simulationStep += 1;

        /***
         * Execute all events we received for this step
         */
        while (!eventsToSimulate.empty() and eventsToSimulate.front()->simulationStep <= simulationStep) {
            assert(eventsToSimulate.front()->simulationStep >= simulationStep); // Earlier steps should be processed by now
            if (const auto* eventData = std::get_if<Event::PlayerActionEvent>(&eventsToSimulate.front()->data)) {
                if (const auto* data = std::get_if<Action::MovementKeysChangedAction>(&eventData->action.data))
                    playerCharacters[eventData->characterID]->movementKeysChanged(data->keyStates);
                if (const auto* data = std::get_if<Action::AttackCharacterAction>(&eventData->action.data))
                    playerCharacters[eventData->characterID]->startAttacking(data->targetID);
                if (const auto* data = std::get_if<Action::BuyItemAction>(&eventData->action.data))
                    playerCharacters[eventData->characterID]->buyItem(data->item);
                if (const auto* data = std::get_if<Action::UsePotionAction>(&eventData->action.data))
                    playerCharacters[eventData->characterID]->usePotion(data->potion);
                if (const auto* data = std::get_if<Action::UsePositionTargetSkillAction>(&eventData->action.data)) {
                    playerCharacters[eventData->characterID]->useSkill(data->skillNum, 0, data->targetPosition);
                    if (playerCharacters[eventData->characterID]->gameShouldCreateScarecrow()) {
                        allies.emplace_back(std::make_shared<Ally>(newCreepIDCounter, CHARACTERS::SCARECROW, data->targetPosition, tilemap, characterContainer, gen(), FPMNum(PLAYER_ARCHER_CREATE_SCARECROW_HP + PLAYER_ARCHER_CREATE_SCARECROW_LEVELUP_EXTRA_HP * (playerCharacters[eventData->characterID]->getSkillLevel(data->skillNum) - 1))));
                        newCreepIDCounter++;
                    }
                }
                if (const auto* data = std::get_if<Action::UseSelfSkillAction>(&eventData->action.data))
                    playerCharacters[eventData->characterID]->useSkill(data->skillNum, 0, FPMVector2());
                if (const auto* data = std::get_if<Action::UseCharacterTargetSkillAction>(&eventData->action.data))
                    playerCharacters[eventData->characterID]->useSkill(data->skillNum, data->targetID, FPMVector2());
                if (const auto* data = std::get_if<Action::UpgradeSkillAction>(&eventData->action.data))
                    playerCharacters[eventData->characterID]->upgradeSkill(data->skillNum);
            }                
            eventsToSimulate.pop_front();
        }

        /***
         * Spawn new creeps
         */
        if (simulationStep >= 1 && (simulationStep * SIMULATION_TIME_STEP_MS) % (CREEP_SPAWN_COOLDOWN_SEC * 1000 / playerCharacters.size()) == 0)
            spawnCreep(0);
        if (simulationStep * SIMULATION_TIME_STEP_MS >= CREEP_SPAWN_POINT_2_ACTIVATE_MIN * 60 * 1000 && (simulationStep * SIMULATION_TIME_STEP_MS) % (CREEP_SPAWN_COOLDOWN_SEC * 1000 / playerCharacters.size()) == 0)
            spawnCreep(1);
        if (simulationStep * SIMULATION_TIME_STEP_MS >= CREEP_SPAWN_POINT_3_ACTIVATE_MIN * 60 * 1000 && (simulationStep * SIMULATION_TIME_STEP_MS) % (CREEP_SPAWN_COOLDOWN_SEC * 1000 / playerCharacters.size()) == 0)
            spawnCreep(2);

        /***
         * Simulate players and creeps for one step. If a creep died, add it to the deadCreeps list (for rendering)
         */
        for (auto& pc : playerCharacters)
            pc->simulate();
        creeps.remove_if([this](auto& c){
            if (c->isDead()) {
                deadCreeps.push_back(c);
                return true;
            }
            c->simulate();
            if (c->hasReachedGoal()) {
                lives--;
                return true;
            }
            return false;
        });
        allies.remove_if([](auto& a){
            if (a->isDead())
                return true;
            a->simulate();
            return false;
        });

        /***
         * Win and lose conditions
         */
        if (lives <= 0) {
            if (outcome == GAME_OUTCOME::STILL_PLAYING)
                outcome = GAME_OUTCOME::LOST;
            lives = 0;
        }
        if (outcome == GAME_OUTCOME::STILL_PLAYING and guards.size() == 3 and guards[0]->isDead() and guards[1]->isDead() and guards[2]->isDead())
            outcome = GAME_OUTCOME::WON;
    }
}

void Game::spawnCreep(int spawnPointIndex) {
    // If a guard for spawnPointIndex has previously spawned and is now dead, we don't spawn any more creeps
    if (guards.size() > spawnPointIndex and guards[spawnPointIndex]->isDead())
        return;
    auto spawnPosition = characterContainer->findFreeSpawnPosition(tilemap->getCreepSpawnZones()[spawnPointIndex], FPMNum(DEFAULT_CHARACTER_RADIUS), gen);
    // If there has not been a guard yet for spawnPointIndex, spawn that first. Then spawn the rest of the creeps
    if (guards.size() <= spawnPointIndex) {
        guards.emplace_back(std::make_shared<Guard>(newCreepIDCounter, CHARACTERS::SHEEP, spawnPosition, tilemap, characterContainer, gen()));
    } else
        creeps.emplace_back(std::make_shared<Creep>(newCreepIDCounter, (simulationStep * SIMULATION_TIME_STEP_MS) / (CREEP_SPAWN_NEXT_LEVEL_SEC * 1000) + 1, spawnPosition, tilemap, characterContainer, gen()));
    newCreepIDCounter++;
}
