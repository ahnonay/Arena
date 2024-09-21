#include <iostream>
#include "Character.h"
#include "../Constants.h"
#include "SFML/Graphics/Glsl.hpp"
#include "SFML/Graphics/RenderStates.hpp"
#include "SFML/Graphics/Shader.hpp"
#include <fstream>
#include <string>
#include <sstream>

std::vector<std::unique_ptr<AnimatedTileset>> Character::characterTilesets;
std::array<std::unique_ptr<sf::Texture>, static_cast<unsigned int>(CONDITIONS::CONDITIONS_COUNT)> Character::conditionIcons;

void Character::loadStaticResources() {
    characterTilesets.clear();
    characterTilesets.resize(static_cast<unsigned int>(CHARACTERS::CHARACTERS_COUNT) *
                             static_cast<unsigned int>(ANIMATION_STATE::CHARACTER_STATE_COUNT));

    std::string charactersInfoFilePath = "Data/characters/file_info.csv";
    std::ifstream file(charactersInfoFilePath);
    if (!file.is_open())
        throw std::runtime_error("Could not open file " + charactersInfoFilePath);

    // Parsing
    std::string lineBuffer, cellBuffer;
    std::vector<std::string> entries;
    // First lineBuffer is just a comment
    std::getline(file, lineBuffer);
    while (file.good()) {
        try {
            std::getline(file, lineBuffer);
            if (lineBuffer.size() == 0)
                break;
            entries.clear();
            std::stringstream ss(lineBuffer);
            while (std::getline(ss, cellBuffer, ','))
                entries.push_back(cellBuffer);
            unsigned int posInArray = std::stoi(entries[0]) * static_cast<unsigned int>(ANIMATION_STATE::CHARACTER_STATE_COUNT) + std::stoi(entries[1]);
            characterTilesets[posInArray] = std::make_unique<AnimatedTileset>(toStr("Data/characters/", entries[2]), std::stoi(entries[5]),
                std::stoi(entries[6]), std::stoi(entries[7]), std::stoi(entries[3]), std::stoi(entries[4]), std::stof(entries[8]));
        } catch (...) {
            throw std::runtime_error("Malformed file: " + charactersInfoFilePath);
        }
    }
    file.close();

    std::generate(conditionIcons.begin(), conditionIcons.end(), std::make_unique<sf::Texture>);
    conditionIcons[static_cast<unsigned int>(CONDITIONS::IMMOBILE)]->loadFromFile("Data/conditions/immobile.png");
    conditionIcons[static_cast<unsigned int>(CONDITIONS::IMMUNE_TO_DAMAGE)]->loadFromFile("Data/conditions/immune.png");
    conditionIcons[static_cast<unsigned int>(CONDITIONS::CONFUSED)]->loadFromFile("Data/conditions/confused.png");
    conditionIcons[static_cast<unsigned int>(CONDITIONS::ENRAGED)]->loadFromFile("Data/conditions/enraged.png");
    conditionIcons[static_cast<unsigned int>(CONDITIONS::POISONED)]->loadFromFile("Data/conditions/poisoned.png");
    std::for_each(conditionIcons.begin(), conditionIcons.end(),[](auto& t) {t->generateMipmap();});
}

void Character::unloadStaticResources() {
    characterTilesets.clear();
    std::fill(conditionIcons.begin(), conditionIcons.end(), nullptr);
}

Character::Character(std::uint32_t ID, CHARACTERS characterNum, FPMVector2 spawnPosition, const std::shared_ptr<Tilemap>& tilemap, const std::shared_ptr<CharacterContainer>& characterContainer, unsigned int randomSeed) :
        ID(ID), type(characterNum), mapPosition(spawnPosition), curOrientation(ORIENTATIONS::W), animationStep(0.f),
        curAnimationState(ANIMATION_STATE::STOP), tilemap(tilemap), maxMovementPerSecond(0),
        groundRadius(DEFAULT_CHARACTER_RADIUS), maxHP(30), HP(30), attackRange(DEFAULT_CHARACTER_ATTACK_RANGE),
        characterContainer(characterContainer), gen(randomSeed),
        attackDamageHP(0), attackCooldownMS(1000), attackTimer(0), hoverColor(sf::Color::White),
        attackTargetID(ID), animationStepsPerSecondFactor(1.f), conditionPoisonDmgPerSec(0) {
    characterContainer->insert(this, mapPosition, groundRadius);
    healthRect.setFillColor(sf::Color(255, 0, 0));
    characterIDShader.loadFromFile("Data/character_id_shader.glsl", sf::Shader::Type::Fragment); 
    characterIDShader.setUniform("texture", sf::Shader::CurrentTexture);
    characterIDShader.setUniform("characterID", sf::Glsl::Vec3((((ID + 1) >> 16) & 0xFF) / 255.f, (((ID + 1) >> 8) & 0xFF) / 255.f, ((ID + 1) & 0xFF) / 255.f));
    conditionTimers.fill(FPMNum24(0));
}

bool Character::updateDrawables(const sf::FloatRect& frustum, float elapsedSeconds, unsigned int elapsedMSSinceLastSimulationStep) {
    // Update animation, even if the character is not currently visible
    auto curTilesetNum = getTilesetIndex(type, curAnimationState);
    auto maxAnimationStep = characterTilesets[curTilesetNum]->getNumTilesAnimation();
    if (maxAnimationStep > 1) {
        animationStep += elapsedSeconds * characterTilesets[curTilesetNum]->getDefaultAnimationStepsPerSecond() * animationStepsPerSecondFactor;
        if (animationStep >= maxAnimationStep) {
            if (curAnimationState == ANIMATION_STATE::DIE or curAnimationState == ANIMATION_STATE::SPELL or curAnimationState == ANIMATION_STATE::HIT)
                animationStep = maxAnimationStep - 1;
            else
                animationStep -= (int) animationStep;
        }
    }

    auto visible = frustum.contains(tilemap->mapToWorld(mapPosition));
    if (visible) {
        sprite.setTextureRect(characterTilesets[curTilesetNum]->getTileCoordinates(curOrientation, animationStep));
        sprite.setTexture(characterTilesets[curTilesetNum]->getTexture(), false);
        sprite.setOrigin(characterTilesets[curTilesetNum]->getOrigin());

        sprite.setColor(hoverColor);

        // We display the character at its most likely next position, assuming it does not change its course.
        // This might turn out to be wrong later once we received the next events, but helps
        // to make movement look much more natural in most cases.
        FPMVector2 newPosition;
        getNextSimulationPosition(newPosition);
        FPMVector2 mapPositionToRender = mapPosition + (newPosition - mapPosition) *
                                                       FPMNum(std::min(elapsedMSSinceLastSimulationStep, (unsigned int) SIMULATION_TIME_STEP_MS) / (float) SIMULATION_TIME_STEP_MS);
        auto worldPositionToRender = tilemap->mapToWorld(mapPositionToRender);
        sprite.setPosition(worldPositionToRender);
        sprite.setDepth(1.f - ((((unsigned int) mapPositionToRender.y) * tilemap->getWidth() + (float) mapPositionToRender.x) / (tilemap->getHeight() * tilemap->getWidth())));

        healthRect.setSize(sf::Vector2f((float)(HP/maxHP) * 40.f, 5.f));
        healthRect.setPosition({worldPositionToRender.x - 20.f, worldPositionToRender.y - 75.f});
    }
    hoverColor = sf::Color::White;
    return visible;
}

void Character::drawSprite(sf::RenderTarget& target) {
    target.draw(sprite);
}

void Character::drawCharacterID(sf::RenderTarget& target) {
    target.draw(sprite, &characterIDShader);
}

void Character::drawUI(sf::RenderTarget &target) {
    // Above the character sprite, draw health rect...
    target.draw(healthRect);
    // ...and icons for the conditions currently active on this character.
    auto numActiveConditions = std::count_if(conditionTimers.begin(), conditionTimers.end(),
                                             [](const FPMNum24 &timer) { return timer > FPMNum24(0); });
    if (numActiveConditions > 0) {
        auto spritePosition = sprite.getPosition();
        float iconSizeX = 20;
        unsigned int iconsRendered = 0;
        float totalLength = numActiveConditions * iconSizeX + (numActiveConditions - 1) * (iconSizeX / 4.f);
        for (unsigned int c = 0; c < static_cast<int>(CONDITIONS::CONDITIONS_COUNT); c++) {
            if (hasCondition(static_cast<CONDITIONS>(c))) {
                sf::Sprite conditionSprite(*conditionIcons[c]);
                conditionSprite.setPosition({spritePosition.x - (totalLength / 2.f) + iconsRendered * (iconSizeX * 5.f / 4.f), spritePosition.y - 100.f});
                float scale = iconSizeX / conditionIcons[c]->getSize().x;
                conditionSprite.setScale({scale, scale});
                target.draw(conditionSprite);
                iconsRendered += 1;
            }
        }
    }
}

unsigned int Character::getTilesetIndex(CHARACTERS type, ANIMATION_STATE state) {
    auto index = static_cast<unsigned int>(state) + static_cast<unsigned int>(type) * static_cast<unsigned int>(ANIMATION_STATE::CHARACTER_STATE_COUNT);
    if (characterTilesets[index] == nullptr) {
        std::cout << "WARNING: Requested non-existent animation state " << static_cast<unsigned int>(state) << " for character " << static_cast<unsigned int>(type) << std::endl;
        index = static_cast<unsigned int>(ANIMATION_STATE::STOP) + static_cast<unsigned int>(type) * static_cast<unsigned int>(ANIMATION_STATE::CHARACTER_STATE_COUNT);
        if (characterTilesets[index] == nullptr)
            throw std::runtime_error("Can't find STOP animation for character");
    }
    return index;
}

void Character::setAnimationState(ANIMATION_STATE state, float animationStepsPerSecondFactor) {
    // Animations have some precedence rules. For example, the HIT animation is more relevant to show than the WALK animation.
    if (curAnimationState == ANIMATION_STATE::DIE and isDead())
        return;
    if ((curAnimationState == ANIMATION_STATE::SPELL or curAnimationState == ANIMATION_STATE::HIT) and (state == ANIMATION_STATE::RUN or state == ANIMATION_STATE::WALK or state == ANIMATION_STATE::STOP or state == ANIMATION_STATE::ATTACK) and animationStep < characterTilesets[getTilesetIndex(type, curAnimationState)]->getNumTilesAnimation() - 1)
        return;

    if (curAnimationState != state)
        animationStep = 0.f;
    curAnimationState = state;
    if (animationStepsPerSecondFactor > 4.f)
        animationStepsPerSecondFactor = 4.f;
    this->animationStepsPerSecondFactor = animationStepsPerSecondFactor;
}

bool Character::getNextSimulationPosition(FPMVector2 &newPosition) const {
    newPosition = mapPosition;
    bool validMove = true;
    if (!isNull(velocity)) {
        newPosition = mapPosition + velocity * FPMNum{SIMULATION_TIME_STEP_SEC};
        if (!tilemap->inMap(newPosition))
            validMove = false;
        if (validMove) {
            // For simplicity, don't consider radius when checking collision with walls
            for (const auto& obstacle: tilemap->getObstaclesAt(newPosition)) {
                if (obstacle->contains(newPosition)) {
                    validMove = false;
                    break;
                }
            }
        }
        if (validMove) {
            // nearbyCharacters might contain too many candidates, as the tolerance is used in rectangular, not circular fashion
            auto nearbyCharacters = characterContainer->getCharactersAtWithTolerance(newPosition, groundRadius);
            for (const auto& character: *nearbyCharacters) {
                if (character != this && character->checkCollisionWithCircle(newPosition, groundRadius)) {
                    validMove = false;
                    break;
                }
            }
        }
    }
    if (!validMove)
        newPosition = mapPosition;
    return validMove;
}

bool Character::tilesetExists(CHARACTERS type, ANIMATION_STATE state) {
    return characterTilesets[static_cast<unsigned int>(state) + static_cast<unsigned int>(type) * static_cast<unsigned int>(ANIMATION_STATE::CHARACTER_STATE_COUNT)] != nullptr;
}

bool Character::checkCollisionWithCircle(const FPMVector2 &center, FPMNum radius) const {
    auto dist = getLength(mapPosition - center);
    return dist < groundRadius + radius;
}

std::string Character::characterTypeToString(CHARACTERS c) {
    switch (c) {
        case CHARACTERS::KNIGHT:
            return "Knight";
        case CHARACTERS::ARCHER:
            return "Archer";
        case CHARACTERS::BAT:
            return "Bat";
        case CHARACTERS::DINO:
            return "Dino";
        case CHARACTERS::GHOST:
            return "Ghost";
        case CHARACTERS::GNOME:
            return "Gnome";
        case CHARACTERS::MAGE:
            return "Mage";
        case CHARACTERS::MONK:
            return "Monk";
        case CHARACTERS::OGRE:
            return "Ogre";
        case CHARACTERS::ORC:
            return "Orc";
        case CHARACTERS::SPIDER:
            return "Spider";
        case CHARACTERS::WOLF:
            return "Wolf";
        case CHARACTERS::ZOMBIE:
            return "Zombie";
        case CHARACTERS::GREEN_ZOMBIE:
            return "Acid Zombie";
        case CHARACTERS::PINK_ZOMBIE:
            return "Zombie King";
        case CHARACTERS::GREEN_DINO:
            return "Dino King";
        case CHARACTERS::SHEEP:
            return "Guardian";
        case CHARACTERS::CROCODILE:
            return "Crocodile";
        case CHARACTERS::BLUE_KNIGHT:
            return "Ice Knight";
        case CHARACTERS::RED_KNIGHT:
            return "Fire Knight";
        case CHARACTERS::GREEN_KNIGHT:
            return "Vanguard";
        case CHARACTERS::BLACK_KNIGHT:
            return "Executioner";
        case CHARACTERS::DARK_DWARF:
            return "Evil Dwarf";
        case CHARACTERS::GREEN_DWARF:
            return "Dwarf";
        case CHARACTERS::SCARECROW:
            return "Scarecrow";
        default:
            throw std::runtime_error("Can't turn CHARACTERS_COUNT into string");
    }
}

void Character::harm(FPMNum amountHP, std::uint32_t attackerID) {
    if (this->HP <= FPMNum(0))
        return;
    if (hasCondition(CONDITIONS::IMMUNE_TO_DAMAGE))
        amountHP *= FPMNum(0.2);
    this->HP -= amountHP;
    if (this->HP <= FPMNum(0)) {
        characterContainer->remove(this, mapPosition, groundRadius);
        this->HP = FPMNum(0);
        setAnimationState(ANIMATION_STATE::DIE);
        setNull(velocity);
    }
}

void Character::heal(FPMNum amountHP) {
    if (this->HP <= FPMNum(0))
        throw std::runtime_error("Attempting to heal dead character");
    this->HP += amountHP;
    if (this->HP > this->maxHP)
        this->HP = this->maxHP;
}

bool Character::deathAnimationComplete() const {
    if (curAnimationState != ANIMATION_STATE::DIE)
        throw std::runtime_error("Called deathAnimationComplete but character is not currently dying");
    return animationStep >= characterTilesets[getTilesetIndex(type, curAnimationState)]->getNumTilesAnimation() - 1;
}

void Character::killedCreep(FPMNum fractionOfDamageCaused) {
    // Do nothing here. Only relevant in Player subclass.
}

void Character::setOrientationFromVector(const FPMVector2 &direction) {
    auto rotation = static_cast<double>(getAngle(direction));
    int orientationIndex = static_cast<int>(std::round(rotation / 3.141593 * 4));
    switch (orientationIndex) {
        case -4:
        case 4:
            curOrientation = ORIENTATIONS::NW;
            break;
        case -3:
            curOrientation = ORIENTATIONS::N;
            break;
        case -2:
            curOrientation = ORIENTATIONS::NE;
            break;
        case -1:
            curOrientation = ORIENTATIONS::E;
            break;
        case 0:
            curOrientation = ORIENTATIONS::SE;
            break;
        case 1:
            curOrientation = ORIENTATIONS::S;
            break;
        case 2:
            curOrientation = ORIENTATIONS::SW;
            break;
        case 3:
            curOrientation = ORIENTATIONS::W;
            break;
        default:
            throw std::runtime_error("Invalid character orientation in Character::setOrientationFromVector");
    }
}

bool Character::isPlayer() const {
    return false;
}

bool Character::isPlayerOrAlly() const {
    return false;
}

void Character::giveCondition(CONDITIONS condition, FPMNum24 lengthMS, std::uint32_t attackerID, FPMNum data) {
    auto conditionIdx = static_cast<unsigned int>(condition);
    conditionTimers[conditionIdx] = lengthMS;
    conditionAttackerIDs[conditionIdx] = attackerID;
    if (condition == CONDITIONS::POISONED) {
        if (data < FPMNum(0))
            throw std::runtime_error("Must pass damage per second to Character::giveCondition for POISONED condition");
        conditionPoisonDmgPerSec = data;
    }
}

void Character::simulate() {
    /////////////////////////////////////////////////////////////////
    /// Conditions
    /////////////////////////////////////////////////////////////////
    if (this->hasCondition(CONDITIONS::POISONED))
        this->harm(conditionPoisonDmgPerSec * FPMNum(SIMULATION_TIME_STEP_SEC), conditionAttackerIDs[static_cast<unsigned int>(CONDITIONS::POISONED)]);

    // Reduce condition timers
    for (auto &timer : conditionTimers) {
        timer -= FPMNum24(SIMULATION_TIME_STEP_MS);
        if (timer <= FPMNum24(0))
            timer = FPMNum24(0);
    }

    /////////////////////////////////////////////////////////////////
    /// The following is related to rendering, but we actually don't have to recompute this every frame.
    /// So we only do it when velocity or attacking state change (i.e. during a call to simulate)
    /////////////////////////////////////////////////////////////////
    if (attackTargetID != this->ID) {
        setAnimationState(ANIMATION_STATE::ATTACK, 1000.f / static_cast<float>(getAttackCooldownMS()));
        if (characterContainer->isAlive(attackTargetID))  // Target may have been killed in the meantime
            setOrientationFromVector(characterContainer->getCharacterByID(attackTargetID)->getMapPosition() - mapPosition);
    } else {
        auto speed = (float) getLength(velocity);
        if (speed > 3.f && tilesetExists(type, ANIMATION_STATE::RUN))
            setAnimationState(ANIMATION_STATE::RUN, speed - 3.f + 1.f);
        else if (speed > 0.01f)
            setAnimationState(ANIMATION_STATE::WALK, 1.f * speed / 3.5f + 0.5f);
        else
            setAnimationState(ANIMATION_STATE::STOP);

        if (speed > 0.01f)
            setOrientationFromVector(velocity);
    }
}

FPMNum24 Character::getAttackCooldownMS() const {
    if (this->hasCondition(CONDITIONS::ENRAGED))
        return attackCooldownMS * FPMNum24(PLAYER_KNIGHT_RAGE_ATTACK_COOLDOWN_FACTOR);
    else
        return attackCooldownMS;
}
