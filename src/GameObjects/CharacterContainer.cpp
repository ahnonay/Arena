#include "CharacterContainer.h"
#include "Character.h"
#include "../Constants.h"

CharacterContainer::CharacterContainer(const std::shared_ptr<Tilemap>& tileMap) : tileMap(tileMap) {
    characterMap.resize(tileMap->getWidth() * tileMap->getHeight() + 1);
}

std::list<Character*> &CharacterContainer::getCharactersAt(const FPMVector2 &map) {
    auto x = static_cast<int>(map.x);
    auto y = static_cast<int>(map.y);
    if (x < 0 or y < 0 or x >= tileMap->getWidth() or y >= tileMap->getHeight())
        return characterMap[tileMap->getWidth() * tileMap->getHeight()];
    return characterMap[y * tileMap->getWidth() + x];
}

std::unique_ptr<std::set<Character*>> CharacterContainer::getCharactersAtWithTolerance(const FPMVector2 &map, FPMNum tolerance) {
    auto returnSet = std::make_unique<std::set<Character*>>();
    for (int x = static_cast<int>(map.x - tolerance); x <= static_cast<int>(map.x + tolerance); x++) {
        if (x < 0 or x >= tileMap->getWidth())
            continue;
        for (int y = static_cast<int>(map.y - tolerance); y <= static_cast<int>(map.y + tolerance); y++) {
            if (y < 0 or y >= tileMap->getHeight())
                continue;
            returnSet->insert(characterMap[y * tileMap->getWidth() + x].begin(), characterMap[y * tileMap->getWidth() + x].end());
        }
    }
    return std::move(returnSet);
}

void CharacterContainer::update(Character* c, const FPMVector2 &oldPos, const FPMVector2 &newPos, FPMNum tolerance) {
    // Characters are actually circles, but here we consider rectangles around oldPos and newPos defined by tolerance
    // Thus, more tiles may be marked than are actually covered by character c
    // However, for small tolerances (i.e. small character radius) the error is not so big and doing a "perfect" check
    // would probably be more expensive
    auto oldCoveredLeft = static_cast<int>(oldPos.x - tolerance);
    auto oldCoveredRight = static_cast<int>(oldPos.x + tolerance);
    auto oldCoveredTop = static_cast<int>(oldPos.y - tolerance);
    auto oldCoveredBottom = static_cast<int>(oldPos.y + tolerance);
    auto newCoveredLeft = static_cast<int>(newPos.x - tolerance);
    auto newCoveredRight = static_cast<int>(newPos.x + tolerance);
    auto newCoveredTop = static_cast<int>(newPos.y - tolerance);
    auto newCoveredBottom = static_cast<int>(newPos.y + tolerance);
    for (int x = std::min(oldCoveredLeft, newCoveredLeft); x <= std::max(oldCoveredRight, newCoveredRight); x++) {
        if (x < 0 or x >= tileMap->getWidth())
            continue;
        for (int y = std::min(oldCoveredTop, newCoveredTop); y <= std::max(oldCoveredBottom, newCoveredBottom); y++) {
            if (y < 0 or y >= tileMap->getHeight())
                continue;
            if (((x < oldCoveredLeft or x > oldCoveredRight) and y >= newCoveredTop and y <= newCoveredBottom) or
                ((y < oldCoveredTop or y > oldCoveredBottom) and x >= newCoveredLeft and x <= newCoveredRight))
                characterMap[y * tileMap->getWidth() + x].push_back(c);
            else if (((x < newCoveredLeft or x > newCoveredRight) and y >= oldCoveredTop and y <= oldCoveredBottom) or
                     ((y < newCoveredTop or y > newCoveredBottom) and x >= oldCoveredLeft and x <= oldCoveredRight))
                characterMap[y * tileMap->getWidth() + x].remove(c);
        }
    }
}

void CharacterContainer::insert(Character* c, const FPMVector2 &pos, FPMNum tolerance) {
    insertOrRemove(c, pos, tolerance, false);
}

void CharacterContainer::remove(Character* c, const FPMVector2 &pos, FPMNum tolerance) {
    insertOrRemove(c, pos, tolerance, true);
}

void CharacterContainer::insertOrRemove(Character* c, const FPMVector2 &pos, FPMNum tolerance, bool remove) {
    if (remove and !c->isPlayer())  // Players are never completely removed
        characterIDs.erase(c->getID());
    else
        characterIDs[c->getID()] = c;
    for (int x = static_cast<int>(pos.x - tolerance); x <= static_cast<int>(pos.x + tolerance); x++) {
        if (x < 0 or x >= tileMap->getWidth())
            continue;
        for (int y = static_cast<int>(pos.y - tolerance); y <= static_cast<int>(pos.y + tolerance); y++) {
            if (y < 0 or y >= tileMap->getHeight())
                continue;
            if (remove)
                characterMap[y * tileMap->getWidth() + x].remove(c);
            else
                characterMap[y * tileMap->getWidth() + x].push_back(c);
        }
    }
}

bool CharacterContainer::isAlive(sf::Uint32 ID) {
    return characterIDs.count(ID) > 0 and !characterIDs[ID]->isDead();
}

FPMVector2 CharacterContainer::findFreeSpawnPosition(const FPMRect &spawnZone, const FPMNum &groundRadius,
                                                     std::mt19937 &gen, unsigned int trials) {
    FPMVector2 spawnPosition;
    std::uniform_int_distribution<int> dist(0, 1000);
    for (unsigned int t = 0; t < trials; t++) {
        spawnPosition.x = spawnZone.left + spawnZone.width * dist(gen) / FPMNum(1000);
        spawnPosition.y = spawnZone.top + spawnZone.height * dist(gen) / FPMNum(1000);
        bool collision = false;
        auto nearbyCharacters = getCharactersAtWithTolerance(spawnPosition, groundRadius);
        for (const auto& character: *nearbyCharacters) {
            if (character->checkCollisionWithCircle(spawnPosition, groundRadius)) {
                collision = true;
                break;
            }
        }
        if (!collision)
            return spawnPosition;
    }
    throw std::runtime_error("Couldn't find free spawn position");
}
