#pragma once

#include "SFML/Graphics.hpp"
#include "../Util.h"
#include <string>
#include <cassert>
#include <iostream>

/***
 * AnimatedTileset stores the texture and some information for a tile sheet.
 * There might be multiple different animations within a tileset. In that case numTilesAnimation stores
 * the number of animation steps per animation (assumed to be the same for all animations).
 * Usually, the tile sheets contain characters and the different animations correspond to the character's
 * orientation (i.e. facing east, west, north etc.).
 */
class AnimatedTileset {
public:
    AnimatedTileset(const std::string &path, int numTilesAnimation, int originX,
                    int originY, int tilesetTileWidth, int tilesetTileHeight, float defaultAnimationStepsPerSecond);

    sf::Vector2f getOrigin() const { return {static_cast<float>(originX), static_cast<float>(originY)}; }

    const sf::Texture &getTexture() const { return texture; }

    unsigned int getNumTilesAnimation() const { return numTilesAnimation; }

    float getDefaultAnimationStepsPerSecond() const { return defaultAnimationStepsPerSecond; }

    sf::IntRect getTileCoordinates(ORIENTATIONS orientation, unsigned int animationStep) const;

    sf::IntRect getTileCoordinates(unsigned int animationIndex, unsigned int animationStep) const;

private:
    unsigned int tilesetTilesPerColumn;
    unsigned int numTilesAnimation;
    unsigned int tilesetTileWidth;
    unsigned int tilesetTileHeight;
    unsigned int originX;
    unsigned int originY;
    float defaultAnimationStepsPerSecond;
    sf::Texture texture;
};
