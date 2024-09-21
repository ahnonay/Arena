#pragma once

#include "Character.h"
#include "Player.h"
#include "../Constants.h"

/***
 * Each Creep object represents one creep displayed in the game.
 *
 * The most important method here is 'simulate', which implements the creeps' behavior.
 * In it, we use a modified version of steering behaviors, which allow to naturally
 * combine different kinds of movements.
 */
class Creep : public Character {
public:
    Creep(std::uint32_t ID, unsigned int level, FPMVector2 spawnPosition, const std::shared_ptr<Tilemap>& tilemap, const std::shared_ptr<CharacterContainer> &characterContainer, unsigned int randomSeed);

    void simulate() override;

    void drawUI(sf::RenderTarget& target) override;

    bool hasReachedGoal() const;

    static CHARACTERS levelToCharacterType(unsigned int creepLevel);

    // In addition to losing HP, here we also keep track of which player caused the damage. If the creep dies, we inform all players that damaged it about their contribution so that they can gain XP etc.
    void harm(FPMNum amountHP, std::uint32_t attackerID) override;
private:
    std::array<FPMNum, MAX_NUM_PLAYERS + 1> damageReceived;

    // In addition to an attack target (see Character class), creeps may have a seek target (a player they move towards, which is not yet in attack range)
    FPMNum seekRange;
    std::uint32_t seekTargetID;

    // If a creep gets start, it starts to move in random directions after a while
    FPMNum stuckTimer;
    //////////////////////
    // Steering behaviors:
    //////////////////////
    // Move towards a target with full speed
    FPMVector2 seek(const FPMVector2 &target);
    // Move away from a target with full speed
    FPMVector2 flee(const FPMVector2 &target);
    // Move towards a target with full speed, getting slower while approaching
    FPMVector2 arrive(const FPMVector2 &target, FPMNum slowingDistance, FPMNum tolerance);
    // Move towards a target while extrapolating its current movement
    FPMVector2 pursue(const Character &target);
    // Move away from a target while extrapolating its current movement
    FPMVector2 evade(const Character &target);
    // Follow a field of direction vectors (provided by Tilemap)
    FPMVector2 followFlowfield();
    // Random movement
    FPMVector2 wander();
    FPMNum wanderAngle;
    // Move cohesively with other nearby creeps
    FPMVector2 flocking();
    // Keep away from other nearby creeps
    FPMVector2 separation();
    // Keep away from rectangular obstacles (provided by Tilemap)
    FPMVector2 avoidObstacles();

    // For debugging
    FPMVector2 seekVelocity, flowfieldVelocity, wanderVelocity, separationVelocity, obstaclesVelocity;

    //FPMVector2 avoidCircleObstacles(const CircleObstacleContainer *obstacles);
    //FPMVector2 avoidWalls(const std::vector<std::shared_ptr<FPMRect>>& obstacles);
    //FPMVector2 avoidCharacters(const std::vector<std::shared_ptr<Character>>& Characters);
    //FPMVector2 followPath();
    //Path CurrentPath;
};
