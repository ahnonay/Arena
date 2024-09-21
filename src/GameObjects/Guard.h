#pragma once

#include "Character.h"

/***
 * A guard is a special kind of creep that stands at the spawn point,
 * has lots of HP, never moves and never attacks. Must be killed to win the game.
 * Thus, this class is very simple (the simulate function is just empty).
 */
class Guard : public Character {
public:
    Guard(std::uint32_t ID, CHARACTERS characterNum, FPMVector2 spawnPosition, const std::shared_ptr<Tilemap>& tilemap, const std::shared_ptr<CharacterContainer> &characterContainer, unsigned int randomSeed);

    void simulate() override;
};
