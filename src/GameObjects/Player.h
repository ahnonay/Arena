#pragma once

#include "Character.h"
#include "Items.h"
#include "Ally.h"

/***
 * Each Player object represents one player in the game. There are only as many player objects as
 * players have joined the game initially. If a player leaves the game, their player object continues to
 * exist, but the character does not take additional actions.
 *
 * In addition to the attributes in the Character class, each player stores some additional values such
 * as XP, MP, potions, skill timers (there is a cooldown after using a skill), respawn timer (after dying,
 * there is a fixed period until the player respawns)...
 * Skills can be upgraded. The initial level of a skill is 1.
 */
class Player : public Character {
public:
    Player(sf::Uint32 ID, CHARACTERS type, FPMVector2 spawnPosition, const std::shared_ptr<Tilemap>& tilemap, const std::shared_ptr<CharacterContainer> &characterContainer, unsigned int randomSeed, std::string name, std::shared_ptr<sf::Font> font);

    void simulate() override;

    bool isPlayer() const override { return true; }

    bool isPlayerOrAlly() const override { return true; }

    bool updateDrawables(const sf::FloatRect& frustum, float elapsedSeconds, unsigned int elapsedMSSinceLastSimulationStep) override;

    void drawUI(sf::RenderTarget& target) override;

    // Stop attacking and move this player according to the given keys pressed.
    void movementKeysChanged(const std::array<bool, 4> &keyStates);

    bool isMoving() const { return !isNull(velocity); }

    // This starts the default attack that every player has. For skills, see below.
    void startAttacking(sf::Uint32 targetID);

    // Check if target is within attackRange and (optionally) if player has clear line of sight
    bool canAttack(const FPMVector2& targetPosition, bool lineOfSightCheck = true);

    // Check if target is alive, within attackRange and (optionally) if player has clear line of sight
    bool canAttack(sf::Uint32 targetID, bool lineOfSightCheck = true);

    // The player's name as input on the main menu
    const std::string &getName() const { return name; }

    unsigned int getLevel() const { return level; }

    const FPMNum &getMp() const { return MP; }

    const FPMNum &getMaxMp() const { return maxMP; }

    const FPMNum &getXp() const { return XP; }

    unsigned int getXpForNextLevel() const;

    unsigned int getGold() const { return gold; }

    // This timer starts counting down once the player has died. At the end, the player respawns
    const FPMNum24 &getRespawnTimer() const { return respawnTimer; }

    const FPMNum24 &getRespawnCooldownMS() const { return respawnCooldownMS; }

    void killedCreep (FPMNum fractionOfDamageCaused) override;

    unsigned int getNumHPPotions() const { return numHPPotions; }

    unsigned int getNumMPPotions() const { return numMPPotions; }

    bool canUsePotion(POTIONS potion);

    bool canBuyItem(ITEMS item);

    void usePotion(POTIONS potion);

    void buyItem(ITEMS item);

    // This timer starts counting down once the player has used a skill. Once it ends, the player can use the skill again.
    float getSkillTimerSec(unsigned int skillSlot) const { return static_cast<float>(skillsTimer[skillSlot - 1] / FPMNum24(1000)); }

    float getSkillCooldownSec(unsigned int skillSlot) const { return static_cast<float>(skillsCooldownMS[skillSlot - 1] / FPMNum24(1000)); }

    // When calling, can set dummy values for targetID or targetPosition, depending on whether the used skill actually needs them
    bool canUseSkill(unsigned int skillSlot, sf::Uint32 targetID, const FPMVector2& targetPosition);

    // When calling, can set dummy values for targetID or targetPosition, depending on whether the used skill actually needs them
    void useSkill(unsigned int skillSlot, sf::Uint32 targetID, const FPMVector2& targetPosition);

    bool canUpgradeSkill(unsigned int skillSlot);

    void upgradeSkill(unsigned int skillSlot);

    // The information in the tooltip can change depending on the player stats (e.g., cooldown time is reduced by certain items), so this can not be a static function.
    std::string getSkillTooltipText(unsigned int skillSlot);

    // Skills can be upgraded, so this gives the current level of a skill for a player. The initial level is always 1.
    unsigned int getSkillLevel(unsigned int skillSlot) const { return skillsLevel[skillSlot]; }

    // Every player can have at most one "zone" (e.g., the Mage's or Monk's death zone). The zone exists until the corresponding timer reaches 0.
    bool isZoneActive() const { return zoneTimer > FPMNum24(0); }

    const FPMVector2& getZonePosition() const { return zonePosition; }

    FPMNum getZoneRadius() const;

    // This is a hack. Since the game class is responsible for managing Ally objects, we can't directly create the Ally in Player::useSkill, but instead inform the Game class that it needs to do this for us. This is of course ugly and it would be better to have CharacterContainer manage the Ally objects.
    bool gameShouldCreateScarecrow();
private:
    void gainXp (FPMNum amount);

    void regainMP (FPMNum amount);

    void makeAOEAttack(const FPMVector2& where, FPMNum radius, FPMNum damageAmount);

    sf::Text nameForRendering;
    std::shared_ptr<sf::Font> font;

    std::string name;
    FPMNum24 respawnTimer;
    FPMNum24 respawnCooldownMS;
    FPMNum maxMP;
    FPMNum MP;
    FPMNum XP;
    unsigned int gold;
    unsigned int level;
    unsigned int numHPPotions;
    unsigned int numMPPotions;

    // Hack. See gameShouldCreateScarecrow()
    bool createScarecrowFlag;

    FPMVector2 zonePosition;
    FPMNum24 zoneTimer;

    std::array<FPMNum24, 7> skillsTimer;
    std::array<FPMNum24, 7> skillsCooldownMS;
    std::array<unsigned int, 7> skillsLevel;
};