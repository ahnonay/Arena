#include "Arrow.h"

#define ARROW_NUM_ORIENTATIONS 32
#define ARROW_SPEED_WORLD_PER_SECOND 600

std::unique_ptr<AnimatedTileset> Arrow::arrowTileset;

void Arrow::loadStaticResources() {
    arrowTileset = std::make_unique<AnimatedTileset>("Data/arrow.png", 1, 16, 58, 32, 64, 1.f);
}

void Arrow::unloadStaticResources() {
    arrowTileset = nullptr;
}

Arrow::Arrow(const std::shared_ptr<Tilemap> &tilemap, FPMVector2 startPosition, FPMVector2 endPosition) :
    Effect(tilemap), startPositionWorld(tilemap->mapToWorld(startPosition)), endPositionWorld(tilemap->mapToWorld(endPosition)),
    totalElapsedSeconds(0.f) {
    auto direction = endPositionWorld - startPositionWorld;
    auto lengthSq = direction.x * direction.x + direction.y + direction.y;
    int orientationIndex;
    if (lengthSq == 0.f) {
        timeToTargetSeconds = 0.f;
        orientationIndex = 0;
    } else {
        timeToTargetSeconds = std::sqrt(lengthSq) / ARROW_SPEED_WORLD_PER_SECOND;
        auto angle = std::atan2(direction.y, direction.x);
        orientationIndex = static_cast<int>(std::round(angle / 3.141593 * (float) (ARROW_NUM_ORIENTATIONS / 2))) +
                ARROW_NUM_ORIENTATIONS / 4;
        if (orientationIndex < 0)
            orientationIndex += 32;
    }
    sprite.setTextureRect(arrowTileset->getTileCoordinates(orientationIndex, 0));
    sprite.setTexture(arrowTileset->getTexture(), false);
    sprite.setOrigin(arrowTileset->getOrigin());
}

bool Arrow::update(const sf::FloatRect &frustum, float elapsedSeconds, unsigned int curSimulationStep) {
    totalElapsedSeconds += elapsedSeconds;
    auto progress = totalElapsedSeconds / timeToTargetSeconds;
    sf::Vector2f worldPositionToRender;
    if (progress >= 1.f)
        worldPositionToRender = endPositionWorld;
    else
        worldPositionToRender = startPositionWorld + progress * (endPositionWorld - startPositionWorld);
    auto visible = frustum.contains(worldPositionToRender);
    if (visible) {
        sprite.setPosition(worldPositionToRender);
        auto mapPositionToRender = tilemap->worldToMap(worldPositionToRender);
        sprite.setDepth(1.f - ((((unsigned int) mapPositionToRender.y) * tilemap->getWidth() + (float) mapPositionToRender.x) / (tilemap->getHeight() * tilemap->getWidth())));
    }
    return visible;
}

void Arrow::draw(sf::RenderTarget &target) {
    target.draw(sprite);
}

bool Arrow::effectHasEnded() const {
    return totalElapsedSeconds >= timeToTargetSeconds;
}
