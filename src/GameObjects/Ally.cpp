#include "Ally.h"

Ally::Ally(sf::Uint32 ID, CHARACTERS characterNum, FPMVector2 spawnPosition, const std::shared_ptr<Tilemap> &tilemap,
             const std::shared_ptr<CharacterContainer> &characterContainer, unsigned int randomSeed, FPMNum maximumHP) : Character(ID, characterNum, spawnPosition, tilemap, characterContainer, randomSeed) {
    maxHP = maximumHP;
    HP = maxHP;
    curOrientation = ORIENTATIONS::S;
}

void Ally::simulate() {
}
