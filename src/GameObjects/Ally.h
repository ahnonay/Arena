#pragma once

#include "Character.h"

/***
 * An ally is a special character that can be created by players.
 * Currently, allies can only stand around and attract creeps. Later, they might also move and attack.
 * Thus, this class is currently very simple (the simulate function is just empty).
 */
class Ally : public Character {
public:
    Ally(std::uint32_t ID, CHARACTERS characterNum, FPMVector2 spawnPosition, const std::shared_ptr<Tilemap>& tilemap, const std::shared_ptr<CharacterContainer> &characterContainer, unsigned int randomSeed, FPMNum maximumHP);

    void simulate() override;

    bool isPlayer() const override { return false; }

    bool isPlayerOrAlly() const override { return true; }
};
