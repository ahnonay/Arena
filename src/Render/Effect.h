#pragma once

#include "SFML/Graphics.hpp"
#include "../GameObjects/Tilemap.h"

/***
 * Special effects like spells or arrows. Are rendered into the map but have no gameplay impact.
 * This superclass has no functionality. Subclasses must implement update (e.g., update animations), draw,
 * and effectHasEnded. The latter returns true if e.g. the spell has ended or the arrow has hit.
 * The corresponding effect object is then destroyed in the Game class.
 */
class Effect {
public:
    explicit Effect(const std::shared_ptr<Tilemap>& tilemap);

    virtual ~Effect() = default;

    virtual bool update(const sf::FloatRect& frustum, float elapsedSeconds, unsigned int curSimulationStep);

    virtual void draw(sf::RenderTarget& target);

    virtual bool effectHasEnded() const;

protected:
    const std::shared_ptr<Tilemap> tilemap;
};
