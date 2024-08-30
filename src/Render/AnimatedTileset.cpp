#include "AnimatedTileset.h"

AnimatedTileset::AnimatedTileset(const std::string &path, int numTilesAnimation, int originX, int originY,
                                 int tilesetTileWidth, int tilesetTileHeight, float defaultAnimationStepsPerSecond) {
    if (!this->texture.loadFromFile(path))
        throw std::runtime_error(toStr("Could not load tileset ", path));
    assert(texture.getSize().x % tilesetTileWidth == 0 && texture.getSize().y % tilesetTileHeight == 0);
    this->tilesetTilesPerColumn = texture.getSize().x / tilesetTileWidth;
    // unsigned int numRows = texture.getSize().y / tilesetTileHeight;
    // assert(numRows * tilesetTilesPerColumn >= numTilesAnimation * static_cast<unsigned int>(ORIENTATIONS::NUM_ORIENTATIONS));
    this->numTilesAnimation = numTilesAnimation;
    this->originX = originX;
    this->originY = originY;
    this->tilesetTileWidth = tilesetTileWidth;
    this->tilesetTileHeight = tilesetTileHeight;
    this->defaultAnimationStepsPerSecond = defaultAnimationStepsPerSecond;
}

sf::IntRect AnimatedTileset::getTileCoordinates(ORIENTATIONS orientation, unsigned int animationStep) const {
    return getTileCoordinates(static_cast<unsigned int>(orientation), animationStep);
}

sf::IntRect AnimatedTileset::getTileCoordinates(unsigned int animationIndex, unsigned int animationStep) const {
    assert(animationStep < numTilesAnimation);
    unsigned int tile = animationStep + numTilesAnimation * animationIndex;
    return sf::IntRect((tile % tilesetTilesPerColumn) * tilesetTileWidth,
                       (tile / tilesetTilesPerColumn) * tilesetTileHeight,
                       tilesetTileWidth, tilesetTileHeight);
}
