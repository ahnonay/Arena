#pragma once

#include <variant>
#include <array>
#include <SFML/Network.hpp>
#include "../Util.h"
#include "../GameObjects/Items.h"

/***
 * Actions are things done by players on their machine. They do not immediately cause things to happen in the game
 * and do not have a time stamp. Instead, they are sent to the server where they are turned into an Event for
 * the next simulationStep (see Event.h).
 *
 * For adding new actions, see the corresponding advice in Event.h.
 */
class Action {
public:
    struct Empty { };
    struct MovementKeysChangedAction { std::array<bool, 4> keyStates; };
    struct AttackCharacterAction { std::uint32_t targetID; };
    struct BuyItemAction { ITEMS item; };
    struct UsePotionAction { POTIONS potion; };
    struct UseSkillAction { std::uint32_t skillNum; };
    struct UseCharacterTargetSkillAction : public UseSkillAction { std::uint32_t targetID; };
    struct UsePositionTargetSkillAction : public UseSkillAction { FPMVector2 targetPosition; };
    struct UseSelfSkillAction : public UseSkillAction {};
    struct UpgradeSkillAction { std::uint32_t skillNum; };

    Action() : data(Empty()) { }

    template <typename T> explicit Action(const T& t) : data(t) { }

    std::variant<Empty, MovementKeysChangedAction, AttackCharacterAction, BuyItemAction, UsePotionAction, UseCharacterTargetSkillAction, UsePositionTargetSkillAction, UseSelfSkillAction, UpgradeSkillAction> data;
};

sf::Packet& operator <<(sf::Packet& packet, const Action& a);
sf::Packet& operator >>(sf::Packet& packet, Action& a);