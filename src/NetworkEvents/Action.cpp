#include "Action.h"

sf::Packet& operator <<(sf::Packet& packet, const Action& a) {
    if (std::holds_alternative<Action::Empty>(a.data))
        throw std::runtime_error("Cannot serialize empty action into packet");
    packet << static_cast<sf::Uint8>(a.data.index());
    if (const auto* data = std::get_if<Action::MovementKeysChangedAction>(&a.data))
        packet << data->keyStates[0] << data->keyStates[1] << data->keyStates[2] << data->keyStates[3];
    if (const auto* data = std::get_if<Action::AttackCharacterAction>(&a.data))
        packet << data->targetID;
    if (const auto* data = std::get_if<Action::BuyItemAction>(&a.data))
        packet << static_cast<sf::Uint8>(data->item);
    if (const auto* data = std::get_if<Action::UsePotionAction>(&a.data))
        packet << static_cast<sf::Uint8>(data->potion);
    if (const auto* data = std::get_if<Action::UseCharacterTargetSkillAction>(&a.data))
        packet << data->skillNum << data->targetID;
    if (const auto* data = std::get_if<Action::UsePositionTargetSkillAction>(&a.data))
        packet << data->skillNum << static_cast<sf::Int32>(data->targetPosition.x.raw_value()) << static_cast<sf::Int32>(data->targetPosition.y.raw_value());
    if (const auto* data = std::get_if<Action::UseSelfSkillAction>(&a.data))
        packet << data->skillNum;
    if (const auto* data = std::get_if<Action::UpgradeSkillAction>(&a.data))
        packet << data->skillNum;
    return packet;
}

sf::Packet& operator >>(sf::Packet& packet, Action& a) {
    sf::Uint8 variantIndex;
    packet >> variantIndex;
    if (variantIndex == 0)
        throw std::runtime_error("Cannot deserialize empty action from packet");
    a.data = expand_type(variantIndex, decltype(a.data)());
    if (auto* data = std::get_if<Action::MovementKeysChangedAction>(&a.data))
        packet >> data->keyStates[0] >> data->keyStates[1] >> data->keyStates[2] >> data->keyStates[3];
    if (auto* data = std::get_if<Action::AttackCharacterAction>(&a.data))
        packet >> data->targetID;
    if (auto* data = std::get_if<Action::BuyItemAction>(&a.data)) {
        sf::Uint8 item;
        packet >> item;
        data->item = static_cast<ITEMS>(item);
    }
    if (auto* data = std::get_if<Action::UsePotionAction>(&a.data)) {
        sf::Uint8 potion;
        packet >> potion;
        data->potion = static_cast<POTIONS>(potion);
    }
    if (auto* data = std::get_if<Action::UseCharacterTargetSkillAction>(&a.data))
        packet >> data->skillNum >> data->targetID;
    if (auto* data = std::get_if<Action::UsePositionTargetSkillAction>(&a.data)) {
        sf::Int32 x, y;
        packet >> data->skillNum >> x >> y;
        data->targetPosition.x = FPMNum::from_raw_value(x);
        data->targetPosition.y = FPMNum::from_raw_value(y);
    }
    if (auto* data = std::get_if<Action::UseSelfSkillAction>(&a.data))
        packet >> data->skillNum;
    if (auto* data = std::get_if<Action::UpgradeSkillAction>(&a.data))
        packet >> data->skillNum;
    return packet;
}
