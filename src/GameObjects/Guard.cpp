#include "Guard.h"
#include "../Constants.h"

Guard::Guard(sf::Uint32 ID, CHARACTERS characterNum, FPMVector2 spawnPosition, const std::shared_ptr<Tilemap> &tilemap,
             const std::shared_ptr<CharacterContainer> &characterContainer, unsigned int randomSeed) : Character(ID, characterNum, spawnPosition, tilemap, characterContainer, randomSeed) {
    maxHP = FPMNum(CREEP_SPAWN_POINT_HP);
    HP = maxHP;
}

void Guard::simulate() {
}
