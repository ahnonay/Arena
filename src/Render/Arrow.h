#pragma once

#include "Effect.h"
#include "../FPMUtil.h"
#include "Sprite3.h"
#include "AnimatedTileset.h"
#include "../GameObjects/Character.h"

/***
 * An arrow for rendering. Has no gameplay relevance, just for show.
 * Has a start and end position and the arrow moves on a straight line between these two.
 * effectHasEnded returns true if the end position was reached.
 */
class Arrow : public Effect {
public:
    Arrow(const std::shared_ptr<Tilemap>& tilemap, FPMVector2 startPosition, FPMVector2 endPosition);

    ~Arrow() override = default;

    static void loadStaticResources();

    static void unloadStaticResources();

    bool update(const sf::FloatRect& frustum, float elapsedSeconds, unsigned int curSimulationStep) override;

    void draw(sf::RenderTarget& target) override;

    bool effectHasEnded() const override;

private:
    static std::unique_ptr<AnimatedTileset> arrowTileset;
    Sprite3 sprite;
    sf::Vector2f startPositionWorld;
    sf::Vector2f endPositionWorld;
    float totalElapsedSeconds;
    float timeToTargetSeconds;
};
