#include "MapCircleShape.h"

MapCircleShape::MapCircleShape(const std::shared_ptr<Tilemap>& tilemap, FPMNum radius, std::size_t pointCount) :
        radius(radius + 1), pointCount(pointCount), tilemap(tilemap) {
    offset = tilemap->mapToWorld(sf::Vector2f(0,0));
    setRadius(radius);
}

void MapCircleShape::setRadius(FPMNum r) {
    if (r != radius) {
        this->radius = r;
        update();
        this->setOrigin(this->getLocalBounds().getPosition() + (this->getLocalBounds().getSize() / 2.f));
    }
}


FPMNum MapCircleShape::getRadius() const {
    return radius;
}

std::size_t MapCircleShape::getPointCount() const {
    return pointCount;
}

sf::Vector2f MapCircleShape::getPoint(std::size_t index) const {
    static const float pi = 3.141592654f;

    auto fradius = static_cast<float>(radius);
    float angle = static_cast<float>(index) * 2.f * pi / static_cast<float>(pointCount) - pi / 2.f;
    float x = std::cos(angle) * fradius;
    float y = std::sin(angle) * fradius;
    
    return tilemap->mapToWorld(sf::Vector2f(fradius + x, fradius + y)) - offset;
}