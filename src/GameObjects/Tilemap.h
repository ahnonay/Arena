#pragma once

#include <list>
#include <iostream>
#include "SFML/Graphics.hpp"
#include "tmxlite/Map.hpp"
#include "../Render/Vertex3.h"
#include "../Util.h"
#include "../FPMUtil.h"

/**
 * The Tilemap class stores all all information about the game world that can be read from the map.tmx file.
 * This information does !not! change over the course of the game. It includes spawn positions for players
 * and creeps, obstacles (modeled as rectangles), the flow field on which creeps are moving, ...
 * Characters and other things that change over the course of the game are not stored here!
 *
 * mapToWorld and worldToMap can be used to convert between the two coordinate systems.
 *
 * The class extends sf::Drawable, so the tilemap can easily be drawn using window->draw(tilemap).
 * Rendering is done efficiently with a single vertex array for the entire map.
 */
class Tilemap : public sf::Drawable {
public:
    explicit Tilemap(const std::string &filename);

    template <typename T> inline sf::Vector2f mapToWorld(const sf::Vector2<T> &map) const {
        auto x = static_cast<float>(map.x);
        auto y = static_cast<float>(map.y);
        return {(static_cast<float>(tileWidth) / 2.f) * (x - y + static_cast<float>(width)),
                (static_cast<float>(tileHeight) / 2.f) * (y + x)};
    }

    inline FPMVector2 worldToMap(const sf::Vector2f &world) const {
        auto w = static_cast<float>(width);
        auto tH = static_cast<float>(tileHeight);
        auto tW = static_cast<float>(tileWidth);
        return {static_cast<FPMNum>((world.x / tW) + (world.y / tH) - (w / 2.f)),
                static_cast<FPMNum>((world.y / tH) - (world.x / tW) + (w / 2.f))};
    }

    unsigned int getHeight() const { return height; }

    unsigned int getWidth() const { return width; }

    template <typename T> inline bool inMap(const sf::Vector2<T> &map) const {
        return map.x >= FPMNum(0) and map.y >= FPMNum(0) and map.x < FPMNum(width) and map.y < FPMNum(height);
    }

    const std::vector<FPMVector2>& getPlayerSpawnPositions() const { return playerSpawnPositions; };

    const std::vector<FPMRect>& getCreepSpawnZones() const { return creepSpawnZones; };

    const FPMRect& getPlayerRespawnZone() const { return playerRespawnZone; };

    const FPMRect& getShop() const { return shop; };

    const FPMRect& getHealingZone() const { return healingZone; };

    const FPMRect& getCreepGoal() const { return creepGoal; };

    std::vector<std::shared_ptr<FPMRect>>& getObstaclesAt(const FPMVector2 &map);

    ORIENTATIONS getFlowAt(const FPMVector2 &map);

    /***
     * Check whether there is an uninterrupted (i.e., no obstacle in the way) straight line from rayStart to
     * rayEnd = rayStart + rayLength * rayNormalizedDirection. If there is a collision, information about it is
     * returned via collisionPosition and collisionNormal.
     *
     * @param rayStart Start point for the collision test
     * @param rayNormalizedDirection Direction from the start point. Must be normalized
     * @param rayLength Distance from start to end
     * @param collisionPosition If there is a collision, this returns the coordinates where the collision occurs
     * @param collisionNormal If there is a collision, this returns a normal of the obstacle that was hit at collisionPosition
     * @return true if there is a collision, false otherwise
     */
    bool lineOfSightCheck(const FPMVector2& rayStart, const FPMVector2& rayNormalizedDirection, const FPMNum& rayLength, FPMVector2 &collisionPosition, FPMVector2 &collisionNormal);

private:
    void draw(sf::RenderTarget &target, sf::RenderStates states) const override;

    void createVertices(const tmx::Map &map, unsigned int tilesetIDOffset);

    sf::Texture tilesetTexture;
    std::vector<Vertex3> vertices;
    unsigned int totalNumVertices;

    unsigned int width;
    unsigned int height;
    unsigned int tileWidth;
    unsigned int tileHeight;
    unsigned int tilesetTileWidth;
    unsigned int tilesetTileHeight;
    unsigned int tilesetTilesPerColumn;

    std::vector<FPMVector2> playerSpawnPositions;
    std::vector<FPMRect> creepSpawnZones;
    FPMRect playerRespawnZone;
    FPMRect shop;
    FPMRect healingZone;
    FPMRect creepGoal;
    std::vector<std::shared_ptr<FPMRect>> obstacles;
    std::vector<std::vector<std::shared_ptr<FPMRect>>> mapToObstacles;
    std::vector<ORIENTATIONS> flowfield;
};
