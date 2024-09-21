#pragma once

#include "SFML/Graphics.hpp"
#include <random>
#include <vector>
#include "../Render/AnimatedTileset.h"
#include "SFML/Graphics/RenderTexture.hpp"
#include "SFML/Graphics/Shader.hpp"
#include "Tilemap.h"
#include "CharacterContainer.h"
#include "../Render/Sprite3.h"
#include "../Util.h"
#include "../FPMUtil.h"

enum class CHARACTERS : std::uint8_t {
    ARCHER, BAT, DINO, GHOST, GNOME, KNIGHT, MAGE, MONK, OGRE, ORC, SPIDER, WOLF, ZOMBIE, GREEN_ZOMBIE, PINK_ZOMBIE,
    GREEN_DINO, SHEEP, CROCODILE, BLUE_KNIGHT, RED_KNIGHT, GREEN_KNIGHT, BLACK_KNIGHT, DARK_DWARF, GREEN_DWARF,
    SCARECROW, CHARACTERS_COUNT
};

enum class ANIMATION_STATE {
    ATTACK, DIE, HIT, RUN, SPELL, STOP, WALK, CHARACTER_STATE_COUNT
};

enum class CONDITIONS {
    IMMOBILE, IMMUNE_TO_DAMAGE, CONFUSED, ENRAGED, POISONED, CONDITIONS_COUNT
};

/***
 * Super class for players, creeps, spawn point guards, and allies.
 * There a some attributes related to the game logic (HP, position in the map etc.), stored using integers
 * or fixed point numbers.
 * Other attributes are only related to rendering (e.g. the current step in the animation). These can also
 * be stored as floats.
 *
 * A character can have CONDITIONS, which disappear after a time.
 * */
class Character {
public:
    Character(std::uint32_t ID, CHARACTERS characterNum, FPMVector2 spawnPosition, const std::shared_ptr<Tilemap>& tilemap, const std::shared_ptr<CharacterContainer>& characterContainer, unsigned int randomSeed);

    virtual ~Character() = default;

    virtual void simulate();

    virtual bool isPlayer() const;

    virtual bool isPlayerOrAlly() const;

    // Must be called before drawing the character to perform culling and update the animation.
    virtual bool updateDrawables(const sf::FloatRect& frustum, float elapsedSeconds, unsigned int elapsedMSSinceLastSimulationStep);

    // Draw the character's sprite. Supports depth testing.
    void drawSprite(sf::RenderTarget& target);

    // Draw the character, but use the pixels to encode the ID of the character. This is used in Game::run to determine the character currently hovered with the mouse.
    void drawCharacterID(sf::RenderTarget& target);

    // Draw some UI elements such as a health bar or character name. Does !not! support depth testing, so this should be called after all other elements on the map have already been drawn.
    virtual void drawUI(sf::RenderTarget& target);

    // Makes the character have a certain color tint for the next draw call. Make sure updateDrawables is called after this function.
    void hover(const sf::Color& color) { this->hoverColor = color; }

    // Load character tile sheets to memory
    static void loadStaticResources();

    // Unload character tile sheets from memory
    static void unloadStaticResources();

    static std::string characterTypeToString(CHARACTERS c);

    // Hurt this character. This function is supposed to be called by the attacker.
    virtual void harm (FPMNum amountHP, std::uint32_t attackerID);

    // Heal this character. This function is supposed to be called by the healer.
    void heal (FPMNum amountHP);

    // True if this character collides with another character at 'center' with ground radius 'radius'
    bool checkCollisionWithCircle (const FPMVector2 &center, FPMNum radius) const;

    const FPMVector2& getMapPosition() const { return mapPosition; }

    const FPMVector2& getVelocity() const { return velocity; }

    const FPMNum& getMaxMovementPerSecond() const { return maxMovementPerSecond; }

    bool isDead() const { return HP<=FPMNum(0); }

    std::uint32_t getID() const { return ID; }

    // True if the character has died and the corresponding animation finished. For creeps, we destroy the creep object once the animation has finished.
    bool deathAnimationComplete() const;

    const FPMNum &getHp() const { return HP; }

    const FPMNum &getMaxHp() const { return maxHP; }

    const FPMNum &getGroundRadius() const { return groundRadius; }

    const FPMNum &getAttackRange() const { return attackRange; }

    const FPMNum &getAttackDamageHP() const { return attackDamageHP; }

    // Get the cooldown between two attacks by this character, in milliseconds. This can change due to levelups or conditions like ENRAGED.
    FPMNum24 getAttackCooldownMS() const;

    std::uint32_t getAttackTargetID() const { return attackTargetID; };

    CHARACTERS getType() const { return type; }

    // Inform this character, that a creep that it previously caused damage to, has died. We use this in the Player class to gain XP and gold. This function is supposed to be called by the dying creep.
    virtual void killedCreep (FPMNum fractionOfDamageCaused);

    ANIMATION_STATE getAnimationState() const { return curAnimationState; }

    // Set the current animation. animationStepsPerSecondFactor can be used to speed up or slow down animations, e.g., to simulate faster walking speed.
    void setAnimationState(ANIMATION_STATE state, float animationStepsPerSecondFactor = 1.f);

    float getAnimationStep() const { return animationStep; }

    bool hasCondition(CONDITIONS condition) const { return conditionTimers[static_cast<unsigned int>(condition)] > FPMNum24(0);}

    // Give this character a 'condition' that ends after 'lengthMS'. The character with 'attackerID' was the cause of the condition. 'data' may contain extra information on the condition, e.g., the strength of the poison in the POISONED condition.
    void giveCondition(CONDITIONS condition, FPMNum24 lengthMS, std::uint32_t attackerID, FPMNum data = FPMNum(-1));

    static const std::unique_ptr<sf::Texture>& getConditionIcon(CONDITIONS condition) { return conditionIcons[static_cast<unsigned int>(condition)]; }
protected:
    static bool tilesetExists(CHARACTERS type, ANIMATION_STATE state);

    // Return whether the character can make a valid move in the next simulation step given its current velocity. Also return the newPosition after that move.
    bool getNextSimulationPosition(FPMVector2& newPosition) const;

    void setOrientationFromVector(const FPMVector2& direction);

    std::mt19937 gen;
    std::uint32_t ID;
    CHARACTERS type;
    FPMVector2 mapPosition;
    FPMVector2 velocity;
    FPMNum maxMovementPerSecond;
    ANIMATION_STATE curAnimationState;
    ORIENTATIONS curOrientation;
    // For collision detection purposes etc., characters are modeled as circles on the ground with a certain radius
    FPMNum groundRadius;
    FPMNum HP;
    FPMNum maxHP;
    FPMNum attackRange;
    FPMNum attackDamageHP;
    // For all characters, there is a certain cooldown between attacks. This is determined by attackCooldownMS
    FPMNum24 attackCooldownMS;
    // attackTimer is set to attackCooldownMS after an attack and needs to go to 0 before the next attack can be made
    FPMNum24 attackTimer;
    // ID of the current target. As a rule, when attackTargetID == ID, there is no current target.
    std::uint32_t attackTargetID;

    // Conditions end after a certain period of time
    std::array<FPMNum24, static_cast<unsigned int>(CONDITIONS::CONDITIONS_COUNT)> conditionTimers;
    std::array<std::uint32_t, static_cast<unsigned int>(CONDITIONS::CONDITIONS_COUNT)> conditionAttackerIDs;
    FPMNum conditionPoisonDmgPerSec;  // Value only relevant if character is currently POISONED
    static std::array<std::unique_ptr<sf::Texture>, static_cast<unsigned int>(CONDITIONS::CONDITIONS_COUNT)> conditionIcons;

    const std::shared_ptr<Tilemap> tilemap;
    const std::shared_ptr<CharacterContainer> characterContainer;

    Sprite3 sprite;
private:
    static std::vector<std::unique_ptr<AnimatedTileset>> characterTilesets;

    static unsigned int getTilesetIndex(CHARACTERS type, ANIMATION_STATE state);

    sf::Color hoverColor;
    sf::Shader characterIDShader;
    float animationStep;
    float animationStepsPerSecondFactor;
    sf::RectangleShape healthRect;
};
