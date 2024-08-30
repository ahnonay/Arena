#pragma once

#include <variant>
#include <SFML/Network.hpp>
#include "../Render/AnimatedTileset.h"
#include "../Util.h"
#include "../GameObjects/Items.h"
#include "Action.h"

/***
 * Events are all the interactions etc. in the game, which do not result deterministically from the
 * objects' states or random seed. Mostly, these correspond to players' Actions.
 * Each event contains the simulationStep that it occurs in. There is a special NoMoreEvents-Event which is
 * the last one sent for a simulationStep. Once this last event has ben received, the corresponding
 * simulationStep can be executed by the game.
 *
 * When adding new types of events, a corresponding subclass must be created, added to the internal std::variant's
 * list of possible values, and code for (de-)serializing the event from/to packets must be written in the
 * corresponding operators (see Event.cpp).
 */
class Event {
public:
    struct Empty { };
    struct PlayerActionEvent { sf::Uint32 characterID; Action action; };
    struct NoMoreEvents { };

    Event() : data(Empty()), simulationStep(0) { }

    // An event is constructed from an object of one of the sub-classes (like PlayerActionEvent) and the event's simulationStep.
    template <typename T> Event(const T& t, sf::Uint32 simulationStep) : data(t), simulationStep(simulationStep) { }

    sf::Uint32 simulationStep;

    std::variant<Empty, PlayerActionEvent, NoMoreEvents> data;
};

sf::Packet& operator <<(sf::Packet& packet, const Event& e);
sf::Packet& operator >>(sf::Packet& packet, Event& e);