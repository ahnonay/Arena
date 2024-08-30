#pragma once
/***
 * Various static information about player skills is stored here. This information does not change
 * over the course of a game.
 */

#include "Character.h"

enum class SKILLS {USE_DISTRACTION, MULTI_ARROW, COLD_ARROW, TANK, CARNAGE, MIGHT, FIREBALL, DEATHZONE_MAGE, ICE_BOMB, HEAL, DEATHZONE_MONK, TELEPORT, CONFUSE, CREATE_SCARECROW, MASS_HEAL, POISON_VIAL, RAGE, NUM_SKILLS};

enum class SKILL_TARGET_TYPES {PASSIVE, SELF, SINGLE_CREEP, RADIUS, SINGLE_ALLY, FREE_SPOT};

struct SkillInfo {
    SKILL_TARGET_TYPES targetType;
    FPMNum radius;
    bool checkLineOfSight;
};

inline SkillInfo getSkillInfo(SKILLS skill) {
    switch(skill) {
        case SKILLS::USE_DISTRACTION:
            return {SKILL_TARGET_TYPES::PASSIVE, FPMNum(0), false};
        case SKILLS::MULTI_ARROW:
            return {SKILL_TARGET_TYPES::RADIUS, FPMNum(PLAYER_ARCHER_MULTIARROW_RADIUS), true};
        case SKILLS::COLD_ARROW:
            return {SKILL_TARGET_TYPES::SINGLE_CREEP, FPMNum(0), true};
        case SKILLS::TANK:
            return {SKILL_TARGET_TYPES::SELF, FPMNum(0), false};
        case SKILLS::CARNAGE:
            return {SKILL_TARGET_TYPES::SELF, FPMNum(0), false};
        case SKILLS::MIGHT:
            return {SKILL_TARGET_TYPES::SINGLE_CREEP, FPMNum(0), true};
        case SKILLS::FIREBALL:
            return {SKILL_TARGET_TYPES::SINGLE_CREEP, FPMNum(0), true};
        case SKILLS::DEATHZONE_MAGE:
            return {SKILL_TARGET_TYPES::RADIUS, FPMNum(PLAYER_MAGE_DEATHZONE_RADIUS), false};
        case SKILLS::ICE_BOMB:
            return {SKILL_TARGET_TYPES::RADIUS, FPMNum(PLAYER_MAGE_ICEBOMB_RADIUS), false};
        case SKILLS::HEAL:
            return {SKILL_TARGET_TYPES::SINGLE_ALLY, FPMNum(0), false};
        case SKILLS::DEATHZONE_MONK:
            return {SKILL_TARGET_TYPES::RADIUS, FPMNum(PLAYER_MONK_DEATHZONE_RADIUS), false};
        case SKILLS::TELEPORT:
            return {SKILL_TARGET_TYPES::FREE_SPOT, FPMNum(0), false};
        case SKILLS::CONFUSE:
            return {SKILL_TARGET_TYPES::RADIUS, FPMNum(PLAYER_MAGE_CONFUSE_RADIUS), false};
        case SKILLS::CREATE_SCARECROW:
            return {SKILL_TARGET_TYPES::FREE_SPOT, FPMNum(0), false};
        case SKILLS::MASS_HEAL:
            return {SKILL_TARGET_TYPES::SELF, FPMNum(0), false};
        case SKILLS::POISON_VIAL:
            return {SKILL_TARGET_TYPES::RADIUS, FPMNum(PLAYER_ARCHER_POISON_VIAL_RADIUS), false};
        case SKILLS::RAGE:
            return {SKILL_TARGET_TYPES::SELF, FPMNum(0), false};
        default:
            throw std::runtime_error("NUM_SKILLS does not have SkillInfo");
    }
}

inline SKILLS skillSlotToSkill(CHARACTERS character, unsigned int skillSlot) {
    switch (character) {
        case CHARACTERS::ARCHER:
            switch (skillSlot) {
                case 1: return SKILLS::USE_DISTRACTION;
                case 2: return SKILLS::MULTI_ARROW;
                case 3: return SKILLS::COLD_ARROW;
                case 4: return SKILLS::CREATE_SCARECROW;
                case 5: return SKILLS::POISON_VIAL;
                default: return SKILLS::NUM_SKILLS;
            }
        case CHARACTERS::KNIGHT:
            switch (skillSlot) {
                case 1: return SKILLS::TANK;
                case 2: return SKILLS::CARNAGE;
                case 3: return SKILLS::MIGHT;
                case 4: return SKILLS::RAGE;
                default: return SKILLS::NUM_SKILLS;
            }
        case CHARACTERS::MAGE:
            switch (skillSlot) {
                case 1: return SKILLS::FIREBALL;
                case 2: return SKILLS::DEATHZONE_MAGE;
                case 3: return SKILLS::ICE_BOMB;
                case 4: return SKILLS::CONFUSE;
                default: return SKILLS::NUM_SKILLS;
            }
        case CHARACTERS::MONK:
            switch (skillSlot) {
                case 1: return SKILLS::HEAL;
                case 2: return SKILLS::DEATHZONE_MONK;
                case 3: return SKILLS::TELEPORT;
                case 4: return SKILLS::MASS_HEAL;
                default: return SKILLS::NUM_SKILLS;
            }
        default:
            throw std::runtime_error("Unknown player character type in skillSlotToSkill");
    }
}

inline FPMNum getMPCostOfSkill(SKILLS skill) {
    switch (skill) {
        case SKILLS::USE_DISTRACTION:
            throw std::runtime_error("USE_DISTRACTION has no MP cost");
        case SKILLS::MULTI_ARROW:
            return FPMNum(PLAYER_ARCHER_MULTIARROW_COST_MP);
        case SKILLS::COLD_ARROW:
            return FPMNum(PLAYER_ARCHER_COOLARROW_COST_MP);
        case SKILLS::TANK:
            return FPMNum(PLAYER_KNIGHT_TANK_COST_MP);
        case SKILLS::CARNAGE:
            return FPMNum(PLAYER_KNIGHT_CARNAGE_COST_MP);
        case SKILLS::MIGHT:
            return FPMNum(PLAYER_KNIGHT_MIGHT_COST_MP);
        case SKILLS::FIREBALL:
            return FPMNum(PLAYER_MAGE_FIREBALL_COST_MP);
        case SKILLS::DEATHZONE_MAGE:
            return FPMNum(PLAYER_MAGE_DEATHZONE_COST_MP);
        case SKILLS::ICE_BOMB:
            return FPMNum(PLAYER_MAGE_ICEBOMB_COST_MP);
        case SKILLS::HEAL:
            return FPMNum(PLAYER_MONK_HEAL_COST_MP);
        case SKILLS::DEATHZONE_MONK:
            return FPMNum(PLAYER_MONK_DEATHZONE_COST_MP);
        case SKILLS::TELEPORT:
            return FPMNum(PLAYER_MONK_TELEPORT_COST_MP);
        case SKILLS::CONFUSE:
            return FPMNum(PLAYER_MAGE_CONFUSE_COST_MP);
        case SKILLS::CREATE_SCARECROW:
            return FPMNum(PLAYER_ARCHER_CREATE_SCARECROW_COST_MP);
        case SKILLS::MASS_HEAL:
            return FPMNum(PLAYER_MONK_MASS_HEAL_COST_MP);
        case SKILLS::POISON_VIAL:
            return FPMNum(PLAYER_ARCHER_POISON_VIAL_COST_MP);
        case SKILLS::RAGE:
            return FPMNum(PLAYER_KNIGHT_RAGE_COST_MP);
        default:
            throw std::runtime_error("NUM_SKILLS has no MP cost");
    }
}

inline unsigned int getMaxSkillLevel(SKILLS skill) {
    switch (skill) {
        case SKILLS::USE_DISTRACTION:
            return PLAYER_ARCHER_DISTRACTION_MAX_LEVEL;
        case SKILLS::MULTI_ARROW:
            return PLAYER_ARCHER_MULTIARROW_MAX_LEVEL;
        case SKILLS::COLD_ARROW:
            return PLAYER_ARCHER_COOLARROW_MAX_LEVEL;
        case SKILLS::TANK:
            return PLAYER_KNIGHT_TANK_MAX_LEVEL;
        case SKILLS::CARNAGE:
            return PLAYER_KNIGHT_CARNAGE_MAX_LEVEL;
        case SKILLS::MIGHT:
            return PLAYER_KNIGHT_MIGHT_MAX_LEVEL;
        case SKILLS::FIREBALL:
            return PLAYER_MAGE_FIREBALL_MAX_LEVEL;
        case SKILLS::DEATHZONE_MAGE:
            return PLAYER_MAGE_DEATHZONE_MAX_LEVEL;
        case SKILLS::ICE_BOMB:
            return PLAYER_MAGE_ICEBOMB_MAX_LEVEL;
        case SKILLS::HEAL:
            return PLAYER_MONK_HEAL_MAX_LEVEL;
        case SKILLS::DEATHZONE_MONK:
            return PLAYER_MONK_DEATHZONE_MAX_LEVEL;
        case SKILLS::TELEPORT:
            return PLAYER_MONK_TELEPORT_MAX_LEVEL;
        case SKILLS::CONFUSE:
            return PLAYER_MAGE_CONFUSE_MAX_LEVEL;
        case SKILLS::CREATE_SCARECROW:
            return PLAYER_ARCHER_CREATE_SCARECROW_MAX_LEVEL;
        case SKILLS::MASS_HEAL:
            return PLAYER_MONK_MASS_HEAL_MAX_LEVEL;
        case SKILLS::POISON_VIAL:
            return PLAYER_ARCHER_POISON_VIAL_MAX_LEVEL;
        case SKILLS::RAGE:
            return PLAYER_KNIGHT_RAGE_MAX_LEVEL;
        default:
            throw std::runtime_error("NUM_SKILLS has no max level");
    }
}

inline std::string getSkillLevelupTooltipText(SKILLS skill) {
    switch (skill) {
        case SKILLS::USE_DISTRACTION:
            return toStr("Increase damage bonus by ", PLAYER_ARCHER_DISTRACTION_LEVELUP_DMG_FACTOR_ADD);
        case SKILLS::MULTI_ARROW:
            throw std::runtime_error("MULTI_ARROW skill has no levelups");
        case SKILLS::COLD_ARROW:
            return toStr("Effect lasts for an additional ", PLAYER_ARCHER_COOLARROW_LEVELUP_EXTRA_SECONDS, "s");
        case SKILLS::TANK:
            return toStr("Effect lasts for an additional ", PLAYER_KNIGHT_TANK_LEVELUP_EXTRA_SECONDS, "s");
        case SKILLS::CARNAGE:
            return toStr("Effect radius increases by ", PLAYER_KNIGHT_CARNAGE_EXTRA_RADIUS, "m");
        case SKILLS::MIGHT:
            return toStr("Increase damage factor by ", PLAYER_KNIGHT_MIGHT_LEVELUP_EXTRA_FACTOR);
        case SKILLS::FIREBALL:
            return toStr("Increase damage factor by ", PLAYER_MAGE_FIREBALL_LEVELUP_EXTRA_FACTOR);
        case SKILLS::DEATHZONE_MAGE:
            return toStr("Effect lasts for an additional ", PLAYER_MAGE_DEATHZONE_LEVELUP_EXTRA_SECONDS, "s");
        case SKILLS::ICE_BOMB:
            return toStr("Effect lasts for an additional ", PLAYER_MAGE_ICEBOMB_EXTRA_SECONDS, "s");
        case SKILLS::HEAL:
            return toStr("Heal an additional ", PLAYER_MONK_HEAL_LEVELUP_EXTRA_HP, " HP");
        case SKILLS::DEATHZONE_MONK:
            return toStr("Effect lasts for an additional ", PLAYER_MONK_DEATHZONE_LEVELUP_EXTRA_SECONDS, "s");
        case SKILLS::TELEPORT:
            throw std::runtime_error("TELEPORT skill has no levelups");
        case SKILLS::CONFUSE:
            return toStr("Effect lasts for an additional ", PLAYER_MAGE_CONFUSE_LEVELUP_EXTRA_SECONDS, "s");
        case SKILLS::CREATE_SCARECROW:
            return toStr("Scarecrow has an additional ", PLAYER_ARCHER_CREATE_SCARECROW_LEVELUP_EXTRA_HP, " HP");
        case SKILLS::MASS_HEAL:
            return toStr("Heal an additional ", PLAYER_MONK_MASS_HEAL_LEVELUP_EXTRA_HP, " HP");
        case SKILLS::POISON_VIAL:
            return toStr("Effect lasts for an additional ", PLAYER_ARCHER_POISON_VIAL_LEVELUP_EXTRA_SECONDS, "s\nand deals extra ", PLAYER_ARCHER_POISON_VIAL_LEVELUP_EXTRA_DMG, " damage");
        case SKILLS::RAGE:
            return toStr("Effect lasts for an additional ", PLAYER_KNIGHT_RAGE_LEVELUP_EXTRA_SECONDS, "s");
        default:
            throw std::runtime_error("NUM_SKILLS has no max level");
    }
}