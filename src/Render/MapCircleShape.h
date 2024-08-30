#pragma once

#include <SFML/Graphics.hpp>
#include "../GameObjects/Tilemap.h"
#include "../FPMUtil.h"

/***
 * A circle in map coordinates corresponds to an ellipsis in world/rendering coordinates.
 * This class allows drawing such an ellipsis based on the radius of the corresponding circle.
 * Does !not! support depth testing (so it is drawn on top of all other objects).
 * Based on sf::CircleShape.
 */
class MapCircleShape : public sf::Shape {
public:
    explicit MapCircleShape(const std::shared_ptr<Tilemap>& tilemap, FPMNum radius, std::size_t pointCount = 100);

    void setRadius(FPMNum r);

    FPMNum getRadius() const;

    std::size_t getPointCount() const override;

    sf::Vector2f getPoint(std::size_t index) const override;
private:
    const std::shared_ptr<Tilemap> tilemap;
    FPMNum radius;
    std::size_t pointCount;
    sf::Vector2f offset;
};