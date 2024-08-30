#include "Effect.h"

Effect::Effect(const std::shared_ptr<Tilemap> &tilemap) : tilemap(tilemap) {

}

bool Effect::update(const sf::FloatRect &frustum, float elapsedSeconds, unsigned int curSimulationStep) {
    throw std::runtime_error("Not implemented");
}

void Effect::draw(sf::RenderTarget &target) {
    throw std::runtime_error("Not implemented");
}

bool Effect::effectHasEnded() const {
    throw std::runtime_error("Not implemented");
}
