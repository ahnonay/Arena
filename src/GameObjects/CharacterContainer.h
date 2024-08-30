#pragma once

#include <random>
#include <vector>
#include <set>
#include "Tilemap.h"
#include "../FPMUtil.h"

class Character;

/***
 * A container that keeps track of all the characters (players and creeps) in the game
 *   - by their ID
 *   - and by their position in the map. This works as a kind of scene graph.
 *
 * The scene graph is always kept up to date and allows for quickly retrieving all characters at a certain
 * position in the map. The fact that characters have a spatial extension is considered as well, so if a character
 * is placed on two tiles at the same tile, getCharactersAt returns it for either of those tiles.
 *
 * Although in the game, characters are modeled as circles on the ground with a groundRadius, we here
 * consider them as squares with a side length of 2*groundRadius. So a character may appear in the list
 * given by getCharactersAt, even though it is not actually on the tile (but very, very close to it).
 * The benefit of this is that update and search operations on the scene graph are much simpler and faster.
 */
class CharacterContainer {
public:
    explicit CharacterContainer(const std::shared_ptr<Tilemap>& tileMap);

    // True if character with ID exists AND is alive.
    bool isAlive(sf::Uint32 ID);

    // Causes exception if ID does not exist. Use isAlive first.
    Character* getCharacterByID(sf::Uint32 ID) { return characterIDs[ID]; }

    // Get characters on that map tile. Fast function.
    std::list<Character*>& getCharactersAt(const FPMVector2 &map);

    // Get characters on that map tile and adjacent tiles up to a Manhatten distance of tolerance. Slow function.
    std::unique_ptr<std::set<Character*>> getCharactersAtWithTolerance(const FPMVector2 &map, FPMNum tolerance);

    // Update the character's position while respecting its spatial extension given by tolerance
    void update(Character* c, const FPMVector2 &oldPos, const FPMVector2 &newPos, FPMNum tolerance);

    // Insert a character into the container for the tile at position pos and for adjacent tiles up to a Manhatten distance of tolerance
    void insert(Character* c, const FPMVector2 &pos, FPMNum tolerance);

    // Remove a character from the container for the tile at position pos and for adjacent tiles up to a Manhatten distance of tolerance. Note that player characters are only removed from the scene graph, but never truly removed from the container (since they later respawn).
    void remove(Character *c, const FPMVector2 &pos, FPMNum tolerance);

    // Randomly (using gen) pick a position in spawnZone, ensuring there is no collision up to groundRadius. Try up to trials times and throw runtime_error if it fails.
    FPMVector2 findFreeSpawnPosition(const FPMRect &spawnZone, const FPMNum &groundRadius, std::mt19937 &gen, unsigned int trials = 10);

private:
    void insertOrRemove(Character* c, const FPMVector2 &pos, FPMNum tolerance, bool remove);

    std::shared_ptr<Tilemap> tileMap;

    std::vector<std::list<Character *>> characterMap;

    std::unordered_map<sf::Uint32, Character*> characterIDs;
};