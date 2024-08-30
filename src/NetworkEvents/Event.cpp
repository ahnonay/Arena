#include "Event.h"

sf::Packet& operator <<(sf::Packet& packet, const Event& e) {
    if (std::holds_alternative<Event::Empty>(e.data))
        throw std::runtime_error("Cannot serialize empty event into packet");
    packet << static_cast<sf::Uint8>(e.data.index());
    packet << e.simulationStep;
    if (const auto* data = std::get_if<Event::PlayerActionEvent>(&e.data))
        packet << data->characterID << data->action;
    return packet;
}

sf::Packet& operator >>(sf::Packet& packet, Event& e) {
    sf::Uint8 variantIndex;
    packet >> variantIndex >> e.simulationStep;
    if (variantIndex == 0)
        throw std::runtime_error("Cannot deserialize empty event from packet");
    e.data = expand_type(variantIndex, decltype(e.data)());
    if (auto* data = std::get_if<Event::PlayerActionEvent>(&e.data))
        packet >> data->characterID >> data->action;
    return packet;
}
