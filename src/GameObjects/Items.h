#pragma once
/***
 * Various static information about items that does not change over the course of the game.
 */

#include "../Constants.h"
#include "../FPMUtil.h"


enum class ITEMS : sf::Uint8 {
    HP_POTION, MP_POTION, HP_TOME, MP_TOME, XP_TOME, MOVEMENT_TOME, ATTACK_COOLDOWN_TOME, ATTACK_DAMAGE_TOME, SKILL_COOLDOWN_TOME, NUM_ITEMS
};

enum class POTIONS : sf::Uint8 {
    HP, MP
};

inline unsigned int getCostOfItem(ITEMS item) {
    switch (item) {
        case ITEMS::HP_POTION:
            return (SHOP_COST_HP_POTION);
        case ITEMS::MP_POTION:
            return (SHOP_COST_MP_POTION);
        case ITEMS::HP_TOME:
            return (SHOP_COST_HP_TOME);
        case ITEMS::MP_TOME:
            return (SHOP_COST_MP_TOME);
        case ITEMS::XP_TOME:
            return (SHOP_COST_XP_TOME);
        case ITEMS::MOVEMENT_TOME:
            return (SHOP_COST_MOVEMENT_TOME);
        case ITEMS::ATTACK_COOLDOWN_TOME:
            return (SHOP_COST_ATTACK_COOLDOWN_TOME);
        case ITEMS::ATTACK_DAMAGE_TOME:
            return (SHOP_COST_ATTACK_DAMAGE_TOME);
        case ITEMS::SKILL_COOLDOWN_TOME:
            return (SHOP_COST_SKILL_COOLDOWN_TOME);
        default:
            throw std::runtime_error("getCostOfItem called with NUM_ITEMS");
    }
}

inline std::string getItemTitle(ITEMS item) {
    switch (item) {
        case ITEMS::HP_POTION:
            return toStr("HP\nPotion\n", SHOP_COST_HP_POTION, " Gold");
        case ITEMS::MP_POTION:
            return toStr("MP\nPotion\n", SHOP_COST_MP_POTION, " Gold");
        case ITEMS::HP_TOME:
            return toStr("Tome of\nHealth\n", SHOP_COST_HP_TOME, " Gold");
        case ITEMS::MP_TOME:
            return toStr("Tome of\nMagic\n", SHOP_COST_MP_TOME, " Gold");
        case ITEMS::XP_TOME:
            return toStr("Tome of\nExperience\n", SHOP_COST_XP_TOME, " Gold");
        case ITEMS::MOVEMENT_TOME:
            return toStr("Tome of\nSpeed\n", SHOP_COST_MOVEMENT_TOME, " Gold");
        case ITEMS::ATTACK_COOLDOWN_TOME:
            return toStr("Tome of\nAgility\n", SHOP_COST_ATTACK_COOLDOWN_TOME, " Gold");
        case ITEMS::ATTACK_DAMAGE_TOME:
            return toStr("Tome of\nMight\n", SHOP_COST_ATTACK_DAMAGE_TOME, " Gold");
        case ITEMS::SKILL_COOLDOWN_TOME:
            return toStr("Tome of\nSkills\n", SHOP_COST_SKILL_COOLDOWN_TOME, " Gold");
        default:
            throw std::runtime_error("getCostOfItem called with NUM_ITEMS");
    }
}

inline std::string getItemTooltip(ITEMS item) {
    switch (item) {
        case ITEMS::HP_POTION:
            return toStr("Heal ", ITEM_HP_POTION_HP_GAIN, " HP");
        case ITEMS::MP_POTION:
            return toStr("Regain ", ITEM_MP_POTION_MP_GAIN, " MP");
        case ITEMS::HP_TOME:
            return toStr("Max HP +", ITEM_HP_TOME_MAX_HP_INCREASE);
        case ITEMS::MP_TOME:
            return toStr("Max MP +", ITEM_MP_TOME_MAX_MP_INCREASE);
        case ITEMS::XP_TOME:
            return toStr("Gain ", ITEM_XP_TOME_XP_GAIN, " XP");
        case ITEMS::MOVEMENT_TOME:
            return toStr("Movement +", ITEM_MOVEMENT_TOME_MOVEMENT_GAIN);
        case ITEMS::ATTACK_COOLDOWN_TOME:
            return toStr("Attack cooldown * ", ITEM_ATTACK_COOLDOWN_TOME_FACTOR);
        case ITEMS::ATTACK_DAMAGE_TOME:
            return toStr("Attack damage * ", ITEM_ATTACK_DAMAGE_TOME_FACTOR);
        case ITEMS::SKILL_COOLDOWN_TOME:
            return toStr("Skill cooldown * ", ITEM_SKILL_COOLDOWN_TOME_FACTOR);
        default:
            throw std::runtime_error("getCostOfItem called with NUM_ITEMS");
    }
}