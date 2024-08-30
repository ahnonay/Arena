#include "Tilemap.h"
#include "SFML/OpenGL.hpp"
#include "tmxlite/TileLayer.hpp"

#include "fpm/ios.hpp"
#include <iostream>
#include "../Constants.h"

Tilemap::Tilemap(const std::string &filename) {
    tmx::Map map;
    if (!map.load(filename))
        throw std::runtime_error("Could not open file " + filename);
    width = map.getTileCount().x;
    height = map.getTileCount().y;
    tileWidth = map.getTileSize().x;
    tileHeight = map.getTileSize().y;
    if(!(map.getTilesets().size() == 2 && map.getTilesets()[0].getName() == "Cave" && map.getTilesets()[1].getName() == "flowfield"))
        throw std::runtime_error("Unknown or missing tilesets in map file " + filename);
    auto tileset = map.getTilesets()[0];
    tilesetTexture.loadFromFile(tileset.getImagePath());
    tilesetTileWidth = tileset.getTileSize().x;
    tilesetTileHeight = tileset.getTileSize().y;
    tilesetTilesPerColumn = tileset.getColumnCount();
    totalNumVertices = 0;
    createVertices(map, tileset.getFirstGID());

    // Note: for some reason, positions and sizes of objects in Tiled .tmx files must be divided by tileHeight
    // See here: https://discourse.mapeditor.org/t/whats-the-algorithm-of-object-position-in-iso-map/1790/3
    for (auto& layer : map.getLayers()) {
        // TODO It is essential that positions etc. are parsed deterministically on all devices. But we get them as floats here...
        if (layer->getName() == "playerSpawn") {
            auto objects = layer->getLayerAs<tmx::ObjectGroup>().getObjects();
            if (objects.size() != MAX_NUM_PLAYERS)
                throw std::runtime_error("Expected MAX_NUM_PLAYERS player spawn points in map");
            playerSpawnPositions.reserve(6);
            for (int index = 0; index < 6; index++) {
                if (std::stoi(objects[5 - index].getName()) != index + 1)
                    throw std::runtime_error("Player spawn points must be named 1, 2, ... and ordered top down");
                playerSpawnPositions[index].x = static_cast<FPMNum>(objects[5 - index].getPosition().x / tileHeight);
                playerSpawnPositions[index].y = static_cast<FPMNum>(objects[5 - index].getPosition().y / tileHeight);
            }
        } else if (layer->getName() == "creepSpawn") {
            auto objects = layer->getLayerAs<tmx::ObjectGroup>().getObjects();
            if (objects.size() != 3)
                throw std::runtime_error("Expected 3 creep spawn points in map");
            creepSpawnZones.reserve(3);
            for (int index = 0; index < 3; index++) {
                if (std::stoi(objects[2 - index].getName()) != index + 1)
                    throw std::runtime_error("Creep spawn points must be named 1, 2, ... and ordered top down");
                creepSpawnZones[index].left = static_cast<FPMNum>(objects[2 - index].getPosition().x / tileHeight);
                creepSpawnZones[index].top = static_cast<FPMNum>(objects[2 - index].getPosition().y / tileHeight);
                creepSpawnZones[index].width = static_cast<FPMNum>(objects[2 - index].getAABB().width / tileHeight);
                creepSpawnZones[index].height = static_cast<FPMNum>(objects[2 - index].getAABB().height / tileHeight);
            }
        } else if (layer->getName() == "playerRespawn") {
            auto objects = layer->getLayerAs<tmx::ObjectGroup>().getObjects();
            if (objects.size() != 1)
                throw std::runtime_error("Expected 1 player respawn zone in map");
            playerRespawnZone.left = static_cast<FPMNum>(objects[0].getPosition().x / tileHeight);
            playerRespawnZone.top = static_cast<FPMNum>(objects[0].getPosition().y / tileHeight);
            playerRespawnZone.width = static_cast<FPMNum>(objects[0].getAABB().width / tileHeight);
            playerRespawnZone.height = static_cast<FPMNum>(objects[0].getAABB().height / tileHeight);
        } else if (layer->getName() == "healingZone") {
            auto objects = layer->getLayerAs<tmx::ObjectGroup>().getObjects();
            if (objects.size() != 1)
                throw std::runtime_error("Expected 1 healing zone in map");
            healingZone.left = static_cast<FPMNum>(objects[0].getPosition().x / tileHeight);
            healingZone.top = static_cast<FPMNum>(objects[0].getPosition().y / tileHeight);
            healingZone.width = static_cast<FPMNum>(objects[0].getAABB().width / tileHeight);
            healingZone.height = static_cast<FPMNum>(objects[0].getAABB().height / tileHeight);
        } else if (layer->getName() == "creepGoal") {
            auto objects = layer->getLayerAs<tmx::ObjectGroup>().getObjects();
            if (objects.size() != 1)
                throw std::runtime_error("Expected 1 creep goal in map");
            creepGoal.left = static_cast<FPMNum>(objects[0].getPosition().x / tileHeight);
            creepGoal.top = static_cast<FPMNum>(objects[0].getPosition().y / tileHeight);
            creepGoal.width = static_cast<FPMNum>(objects[0].getAABB().width / tileHeight);
            creepGoal.height = static_cast<FPMNum>(objects[0].getAABB().height / tileHeight);
        } else if (layer->getName() == "shop") {
            auto objects = layer->getLayerAs<tmx::ObjectGroup>().getObjects();
            if (objects.size() != 1)
                throw std::runtime_error("Expected 1 shop zone in map");
            shop.left = static_cast<FPMNum>(objects[0].getPosition().x / tileHeight);
            shop.top = static_cast<FPMNum>(objects[0].getPosition().y / tileHeight);
            shop.width = static_cast<FPMNum>(objects[0].getAABB().width / tileHeight);
            shop.height = static_cast<FPMNum>(objects[0].getAABB().height / tileHeight);
        } else if (layer->getName() == "obstacles") {
            mapToObstacles.resize((width + 2) * (height + 2) + 1);
            auto objects = layer->getLayerAs<tmx::ObjectGroup>().getObjects();
            obstacles.reserve(objects.size());
            for (const auto& object : objects) {
                assert(object.getShape() == tmx::Object::Shape::Rectangle);
                auto newObstacle = std::make_shared<FPMRect>(static_cast<FPMNum>(object.getAABB().left / tileHeight), static_cast<FPMNum>(object.getAABB().top / tileHeight),
                                                             static_cast<FPMNum>(object.getAABB().width / tileHeight), static_cast<FPMNum>(object.getAABB().height / tileHeight));
                FPMVector2 posInObstacle;
                for (posInObstacle.x = newObstacle->left; posInObstacle.x < newObstacle->left + newObstacle->width; posInObstacle.x += 1) {
                    for (posInObstacle.y = newObstacle->top; posInObstacle.y < newObstacle->top + newObstacle->height; posInObstacle.y += 1) {
                        getObstaclesAt(posInObstacle).push_back(newObstacle);
                    }
                }
                obstacles.emplace_back(newObstacle);
            }
        } else if (layer->getName() == "flowfield") {
            auto firstFlowfieldID = map.getTilesets()[1].getFirstGID();
            flowfield.resize(width * height, ORIENTATIONS::NUM_ORIENTATIONS);
            auto tiles = layer->getLayerAs<tmx::TileLayer>().getTiles();
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    auto tileID = tiles[y * width + x].ID;
                    if (tileID > 0) {
                        if (tileID < firstFlowfieldID || tileID >= firstFlowfieldID + 8)
                            throw std::runtime_error("Invalid tiles in flowfield layer");
                        flowfield[y * width + x] = static_cast<ORIENTATIONS>(tileID - firstFlowfieldID);
                    }
                }
            }
        }
    }
}

void Tilemap::createVertices(const tmx::Map &map, unsigned int tilesetIDOffset) {
    assert(tilesetTileHeight >= tileHeight);
    int startY = tileHeight - tilesetTileHeight;
    unsigned int offX = tileWidth / 2;
    unsigned int offY = tileHeight / 2;
    int startX = ((width - 1) * tileWidth) / 2;

    vertices.resize(width * height * 6 * 3); // Could also reserve based on actual contents

    unsigned int renderOrder[3];
    for (unsigned int i = 0; i < map.getLayers().size(); i++) {
        if (map.getLayers()[i]->getName() == "ground") {
            renderOrder[0] = i;
        } else if (map.getLayers()[i]->getName() == "walls") {
            renderOrder[1] = i;
        } else if (map.getLayers()[i]->getName() == "decor") {
            renderOrder[2] = i;
        }
    }

    totalNumVertices = 0;
    for (unsigned int i: renderOrder) {
        auto layer = map.getLayers()[i]->getLayerAs<tmx::TileLayer>();
        auto tiles = layer.getTiles();
        for (int y = 0; y < height; ++y) {
            int drawX = startX - y * offX;
            int drawY = startY + y * offY;
            for (int x = 0; x < width; ++x) {
                auto tileID = tiles[y * width + x].ID;
                if (tileID > 0) {
                    tileID -= tilesetIDOffset;
                    unsigned int col = tileID / tilesetTilesPerColumn;
                    unsigned int posInCol = tileID - col * tilesetTilesPerColumn;
                    unsigned int tilesetX = posInCol * tilesetTileWidth;
                    unsigned int tilesetY = col * tilesetTileHeight;

                    // https://www.sfml-dev.org/tutorials/2.6/graphics-vertex-array.php
                    Vertex3 *triangles = &vertices[totalNumVertices];

                    float depth = 1.f;
                    if (map.getLayers()[i]->getName() != "ground")
                        depth = 1.f - (((float) y * width + x) / (height * width)); // 1.f - ((float) y / height);
                    triangles[0].position3D = sf::Vector3f(drawX, drawY, depth);
                    triangles[1].position3D = sf::Vector3f(drawX + tilesetTileWidth, drawY, depth);
                    triangles[2].position3D = sf::Vector3f(drawX, drawY + tilesetTileHeight, depth);
                    triangles[3].position3D = sf::Vector3f(drawX, drawY + tilesetTileHeight, depth);
                    triangles[4].position3D = sf::Vector3f(drawX + tilesetTileWidth, drawY, depth);
                    triangles[5].position3D = sf::Vector3f(drawX + tilesetTileWidth, drawY + tilesetTileHeight, depth);

                    triangles[0].texCoords = sf::Vector2f(tilesetX, tilesetY);
                    triangles[1].texCoords = sf::Vector2f(tilesetX + tilesetTileWidth, tilesetY);
                    triangles[2].texCoords = sf::Vector2f(tilesetX, tilesetY + tilesetTileHeight);
                    triangles[3].texCoords = sf::Vector2f(tilesetX, tilesetY + tilesetTileHeight);
                    triangles[4].texCoords = sf::Vector2f(tilesetX + tilesetTileWidth, tilesetY);
                    triangles[5].texCoords = sf::Vector2f(tilesetX + tilesetTileWidth, tilesetY + tilesetTileHeight);

                    totalNumVertices += 6;
                }
                drawX += offX;
                drawY += offY;
            }
        }
    }
}

// https://www.jordansavant.com/book/graphics/sfml/sfml2_depth_buffering.md
// Re-implementation of sf::RenderTarget::draw() to allow depth testing
void Tilemap::draw(sf::RenderTarget &target, sf::RenderStates states) const {
    // If no vertices, do not render
    if (vertices.empty() || totalNumVertices == 0)
        return;
    states.texture = &tilesetTexture;

    // Apply the transform
    glLoadMatrixf(states.transform.getMatrix());

    // We do the following two calls already in Game.render, but leaving them here in case of future issues
    // Apply the view
    //applyCurrentView(target);

    // Apply the blend mode
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Apply the texture
    sf::Texture::bind(states.texture, sf::Texture::Pixels);

    // Apply the shader
    if (states.shader)
        sf::Shader::bind(states.shader);

    // Setup the pointers to the vertices' components
    const char *data = reinterpret_cast<const char *>(&vertices[0]);
    glVertexPointer(3, GL_FLOAT, sizeof(Vertex3), data);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex3), data + 12);
    glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex3), data + 16);

    // Draw the primitives
    glDrawArrays(GL_TRIANGLES, 0, totalNumVertices);

    // Unbind the shader, if any
    if (states.shader)
        sf::Shader::bind(nullptr);
}

std::vector<std::shared_ptr<FPMRect>> &Tilemap::getObstaclesAt(const FPMVector2 &map) {
    if (map.x < FPMNum(-1) or map.y < FPMNum(-1) or map.x >= FPMNum(width + 1) or map.y >= FPMNum(height + 1))
        return mapToObstacles[(width + 2) * (height + 2)];
    else {
        unsigned int x = static_cast<unsigned int>(map.x + 1);
        unsigned int y = static_cast<unsigned int>(map.y + 1);
        return mapToObstacles[y * (width + 2) + x];
    }
}

ORIENTATIONS Tilemap::getFlowAt(const FPMVector2 &map) {
    auto x = static_cast<int>(map.x);
    auto y = static_cast<int>(map.y);
    if (x < 0 or y < 0 or x >= width or y >= height)
        return ORIENTATIONS::NUM_ORIENTATIONS;
    return flowfield[y * width + x];
}

bool Tilemap::lineOfSightCheck(const FPMVector2 &rayStart, const FPMVector2& rayNormalizedDirection,
                               const FPMNum &rayLength, FPMVector2 &collisionPosition, FPMVector2 &collisionNormal) {
    if (isNull(rayNormalizedDirection))
        return false;

    FPMVector2 invDir(FPMNum(0), FPMNum(0));
    if (rayNormalizedDirection.x != FPMNum(0))
        invDir.x = FPMNum(1) / rayNormalizedDirection.x;
    if (rayNormalizedDirection.y != FPMNum(0))
        invDir.y = FPMNum(1) / rayNormalizedDirection.y;
    auto rayEnd = rayStart + rayNormalizedDirection * rayLength;

    // https://stackoverflow.com/a/38552664
    // http://www.cse.yorku.ca/~amana/research/grid.pdf
#define SIGN(x) (x > FPMNum(0) ? FPMNum(1) : (x <  FPMNum(0) ? FPMNum(-1) :  FPMNum(0)))
#define FRAC0(x) (x - fpm::floor(x))
#define FRAC1(x) (FPMNum(1) - x + fpm::floor(x))

    FPMNum tMaxX, tMaxY, tDeltaX, tDeltaY;

    auto dx = SIGN(rayEnd.x - rayStart.x);
    if (dx != FPMNum(0)) tDeltaX = std::min(dx / (rayEnd.x - rayStart.x), FPMNum(1000)); else tDeltaX = FPMNum(1000);
    if (dx > FPMNum(0)) tMaxX = tDeltaX * FRAC1(rayStart.x); else tMaxX = tDeltaX * FRAC0(rayStart.x);

    auto dy = SIGN(rayEnd.y - rayStart.y);
    if (dy != FPMNum(0)) tDeltaY = std::min(dy / (rayEnd.y - rayStart.y), FPMNum(1000)); else tDeltaY = FPMNum(1000);
    if (dy > FPMNum(0)) tMaxY = tDeltaY * FRAC1(rayStart.y); else tMaxY = tDeltaY * FRAC0(rayStart.y);

    auto curTile = FPMVector2(fpm::floor(rayStart.x), fpm::floor(rayStart.y));

    bool collision = false;
    while (!collision) {
        for (const auto& obstacle : getObstaclesAt(curTile)) {
            // Line/AABB collision code: https://tavianator.com/2022/ray_box_boundary.html
            auto t1 = (obstacle->left - rayStart.x)*invDir.x;
            auto t2 = (obstacle->left + obstacle->width - rayStart.x)*invDir.x;
            auto t3 = (obstacle->top - rayStart.y)*invDir.y;
            auto t4 = (obstacle->top + obstacle->height - rayStart.y)*invDir.y;

            auto tmin = std::max(std::min(t1, t2), std::min(t3, t4));
            auto tmax = std::min(std::max(t1, t2), std::max(t3, t4));

            if (tmin <= tmax and tmin <= rayLength and tmax >= FPMNum(0))
            {
                collision = true;
                collisionPosition = rayStart + rayNormalizedDirection * tmin;
                if (tmin == t1) collisionNormal = FPMVector2(FPMNum(-1),  FPMNum(0)); /* left */
                if (tmin == t2) collisionNormal = FPMVector2( FPMNum(1),  FPMNum(0)); /* right */
                if (tmin == t3) collisionNormal = FPMVector2( FPMNum(0), FPMNum(-1)); /* bottom */
                if (tmin == t4) collisionNormal = FPMVector2( FPMNum(0),  FPMNum(1)); /* top */
                break;
            }
        }
        if (tMaxX > FPMNum(1) && tMaxY > FPMNum(1)) break;
        if (tMaxX < tMaxY) {
            curTile.x += dx;
            tMaxX += tDeltaX;
        } else {
            curTile.y += dy;
            tMaxY += tDeltaY;
        }
    }
    return collision;
}