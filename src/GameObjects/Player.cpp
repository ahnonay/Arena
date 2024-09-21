#include "Player.h"

#include <utility>

#include "../Constants.h"
#include "Skills.h"

Player::Player(std::uint32_t ID, CHARACTERS type, FPMVector2 spawnPosition, const std::shared_ptr<Tilemap> &tilemap, const std::shared_ptr<CharacterContainer> &characterContainer, unsigned int randomSeed, std::string name, std::shared_ptr<sf::Font> font)
        : Character(ID, type, spawnPosition, tilemap, characterContainer, randomSeed), name(std::move(name)),
          respawnTimer(0), respawnCooldownMS(PLAYER_RESPAWN_SEC * 1000),
          gold(0), level(1), XP(0), nameForRendering(*font), font(std::move(font)), numHPPotions(0), numMPPotions(0), zoneTimer(0),
          zonePosition(FPMNum(0), FPMNum(0)), maxMP(0), MP(0), skillsCooldownMS(),
          skillsTimer() {
    // Set stats depending on the character type
    maxMovementPerSecond = FPMNum(PLAYER_INITIAL_MAX_MOVEMENT_PER_SEC);
    switch (type) {
        case CHARACTERS::KNIGHT: {
            maxHP = FPMNum(PLAYER_KNIGHT_INITIAL_HP);
            maxMP = FPMNum(PLAYER_KNIGHT_INITIAL_MP);
            attackDamageHP = FPMNum(PLAYER_KNIGHT_INITIAL_ATTACK_DMG);
            attackCooldownMS = FPMNum24(PLAYER_KNIGHT_INITIAL_ATTACK_COOLDOWN_MS);
            attackRange = FPMNum(PLAYER_KNIGHT_ATTACK_RANGE);
        } break;
        case CHARACTERS::ARCHER: {
            maxHP = FPMNum(PLAYER_ARCHER_INITIAL_HP);
            maxMP = FPMNum(PLAYER_ARCHER_INITIAL_MP);
            attackDamageHP = FPMNum(PLAYER_ARCHER_INITIAL_ATTACK_DMG);
            attackCooldownMS = FPMNum24(PLAYER_ARCHER_INITIAL_ATTACK_COOLDOWN_MS);
            attackRange = FPMNum(PLAYER_ARCHER_ATTACK_RANGE);
        } break;
        case CHARACTERS::MAGE: {
            maxHP = FPMNum(PLAYER_MAGE_INITIAL_HP);
            maxMP = FPMNum(PLAYER_MAGE_INITIAL_MP);
            attackDamageHP = FPMNum(PLAYER_MAGE_INITIAL_ATTACK_DMG);
            attackCooldownMS = FPMNum24(PLAYER_MAGE_INITIAL_ATTACK_COOLDOWN_MS);
            attackRange = FPMNum(PLAYER_MAGE_ATTACK_RANGE);
        } break;
        case CHARACTERS::MONK: {
            maxHP = FPMNum(PLAYER_MONK_INITIAL_HP);
            maxMP = FPMNum(PLAYER_MONK_INITIAL_MP);
            attackDamageHP = FPMNum(PLAYER_MONK_INITIAL_ATTACK_DMG);
            attackCooldownMS = FPMNum24(PLAYER_MONK_INITIAL_ATTACK_COOLDOWN_MS);
            attackRange = FPMNum(PLAYER_MONK_ATTACK_RANGE);
        } break;
        default:
            throw std::runtime_error("Player character type must be KNIGHT, MONK, ARCHER, or MAGE");
    }
    HP = maxHP;
    MP = maxMP;
    
    createScarecrowFlag = false;

    // Player names are displayed under the respective character sprites
    this->font->setSmooth(false);
    nameForRendering.setFillColor(sf::Color::White);
    nameForRendering.setFont(*this->font);
    nameForRendering.setString(sf::String::fromUtf8(this->name.begin(), this->name.end()));
    nameForRendering.setCharacterSize(13);
    nameForRendering.setOrigin(nameForRendering.getGlobalBounds().size / 2.f);

    // Most skills have the default cooldown, some take longer
    skillsTimer.fill(FPMNum24(0));
    skillsCooldownMS.fill(FPMNum24(PLAYER_SKILL_BASE_COOLDOWN_SECONDS * 1000));
    if (this->type == CHARACTERS::MAGE)
        skillsCooldownMS[1] *= FPMNum24(PLAYER_MAGE_DEATHZONE_COOLDOWN_FACTOR);
    if (this->type == CHARACTERS::MONK)
        skillsCooldownMS[1] *= FPMNum24(PLAYER_MONK_DEATHZONE_COOLDOWN_FACTOR);
    skillsLevel.fill(1);
}

void Player::movementKeysChanged(const std::array<bool, 4> &keyStates) {
    // Here, we don't need to do plausibility checks (e.g. using getNextSimulationPosition)
    // because this is done in Player::simulate
    bool moving = true;
    ORIENTATIONS newOrientation;
    if (keyStates[1]) { // A
        if (keyStates[2]) // S
            newOrientation = ORIENTATIONS::SW;
        else if (keyStates[0]) // W
            newOrientation = ORIENTATIONS::NW;
        else
            newOrientation = ORIENTATIONS::W;
    } else if (keyStates[3]) { // D
        if (keyStates[2]) // S
            newOrientation = ORIENTATIONS::SE;
        else if (keyStates[0]) // W
            newOrientation = ORIENTATIONS::NE;
        else
            newOrientation = ORIENTATIONS::E;
    } else if (keyStates[2]) // S
        newOrientation = ORIENTATIONS::S;
    else if (keyStates[0]) // W
        newOrientation = ORIENTATIONS::N;
    else
        moving = false;

    if (moving)
        velocity = getDirectionVector(newOrientation) * maxMovementPerSecond;
    else
        setNull(velocity);
    this->attackTargetID = ID;
}

void Player::startAttacking(std::uint32_t targetID) {
    // Here, we don't need to do plausibility checks (e.g. using canAttack)
    // because this is done in Player::simulate
    this->attackTargetID = targetID;
    setNull(velocity);
}

void Player::simulate() {
    // Execute the death zone effect, if the one is active. Note that the zone can be active if the player is dead!
    // Also reduce the zone timer (effect stops once timer reaches 0)
    if (zoneTimer > FPMNum24(0)) {
        zoneTimer -= FPMNum24(SIMULATION_TIME_STEP_MS);
        if (zoneTimer < FPMNum24(0))
            zoneTimer = FPMNum24(0);
        if (this->type == CHARACTERS::MAGE)
            makeAOEAttack(zonePosition, FPMNum(PLAYER_MAGE_DEATHZONE_RADIUS), FPMNum(FPMNum24(PLAYER_MAGE_DEATHZONE_FACTOR * SIMULATION_TIME_STEP_MS) * FPMNum24(attackDamageHP) / getAttackCooldownMS()));
        else // MONK
            makeAOEAttack(zonePosition, FPMNum(PLAYER_MONK_DEATHZONE_RADIUS), FPMNum(FPMNum24(PLAYER_MONK_DEATHZONE_FACTOR * SIMULATION_TIME_STEP_MS) * FPMNum24(attackDamageHP) / getAttackCooldownMS()));
    }

    // If the player is dead, deal with respawning.
    // First, reduce the corresponding timer. And once the timer reaches 0, refill HP etc. and place player on an
    // spot in the spawn region.
    if (this->isDead()) {
        setNull(velocity);
        if (respawnTimer <= FPMNum24(0)) { // just died
            respawnTimer = respawnCooldownMS;
            return;
        } else {
            respawnTimer -= FPMNum24(SIMULATION_TIME_STEP_MS); // respawning
            if (respawnTimer <= FPMNum24(0)) { // just respawned
                mapPosition = characterContainer->findFreeSpawnPosition(tilemap->getPlayerRespawnZone(), groundRadius, gen);
                HP = maxHP;
                MP = maxMP;
                skillsTimer.fill(FPMNum24(0));
                conditionTimers.fill(FPMNum24(0));
                characterContainer->insert(this, mapPosition, groundRadius);
            } else
                return; // still dead? return
        }
    }

    // !!!The following is only executed if the player is alive!!!

    // Handle some conditions
    if (this->hasCondition(CONDITIONS::IMMOBILE))
        setNull(velocity);

    // Reduce skill cooldown timers
    for (auto &timer : skillsTimer) {
        timer -= FPMNum24(SIMULATION_TIME_STEP_MS);
        if (timer < FPMNum24(0))
            timer = FPMNum24(0);
    }

    // MP regeneration
    this->regainMP(FPMNum(PLAYER_MAX_MP_PERCENTAGE_REGEN_SEC * 0.01 * SIMULATION_TIME_STEP_SEC) * maxMP);

    // HP/MP regeneration if player is in healing zone
    if (tilemap->getHealingZone().contains(mapPosition)) {
        this->heal(FPMNum(HEALING_ZONE_MAX_HP_PERCENTAGE_REGEN_SEC * 0.01 * SIMULATION_TIME_STEP_SEC) * maxHP);
        this->regainMP(FPMNum(HEALING_ZONE_MAX_MP_PERCENTAGE_REGEN_SEC * 0.01 * SIMULATION_TIME_STEP_SEC) * maxMP);
    }

    if (attackTargetID != ID) {  // If player is attacking...
        setNull(velocity);
        // Stop attacking if target is out of range or died
        if (!canAttack(attackTargetID, true)) {
            attackTargetID = ID;
        } else {
            // If the cooldown is over, attack. Otherwise, reduce cooldown timer.
            if (attackTimer <= FPMNum24(0)) {
                attackTimer = getAttackCooldownMS();
                FPMNum damageAmount(attackDamageHP);
                // Archers deal extra damage if an ally is close to the target
                if (this->type == CHARACTERS::ARCHER) {
                    bool allyNearby = false;
                    auto nearbyCharacters = characterContainer->getCharactersAtWithTolerance(characterContainer->getCharacterByID(attackTargetID)->getMapPosition(), FPMNum(PLAYER_ARCHER_DISTRACTION_MAX_DISTANCE_TO_ALLY));
                    for (const auto &c: *nearbyCharacters) {
                        if (c->isPlayerOrAlly() and getLength(characterContainer->getCharacterByID(attackTargetID)->getMapPosition() - c->getMapPosition()) <= FPMNum(PLAYER_ARCHER_DISTRACTION_MAX_DISTANCE_TO_ALLY)) {
                            allyNearby = true;
                            break;
                        }
                    }
                    assert(skillSlotToSkill(this->type, 1) == SKILLS::USE_DISTRACTION);
                    if (allyNearby)
                        damageAmount *= FPMNum(PLAYER_ARCHER_DISTRACTION_DMG_FACTOR + PLAYER_ARCHER_DISTRACTION_LEVELUP_DMG_FACTOR_ADD * (skillsLevel[0] - 1));
                }
                characterContainer->getCharacterByID(attackTargetID)->harm(damageAmount, ID);
            } else
                attackTimer -= SIMULATION_TIME_STEP_MS;
        }
    } else if (!isNull(velocity)) { // else, if player is moving...
        // ... move player and update scene graph.
        FPMVector2 newPosition;
        if (getNextSimulationPosition(newPosition)) {
            characterContainer->update(this, mapPosition, newPosition, groundRadius);
            mapPosition = newPosition;
        } else
            setNull(velocity);  // stop moving
    }

    Character::simulate();
}

unsigned int Player::getXpForNextLevel() const {
    return level * 10 * 100;
}

void Player::killedCreep(FPMNum fractionOfDamageCaused) {
    auto gain = fractionOfDamageCaused * FPMNum(XP_GOLD_PER_KILLED_CREEP);
    gold += static_cast<unsigned int>(fpm::round(gain));
    gainXp(gain);
}

void Player::gainXp(FPMNum amount) {
    XP += amount;
    auto forNextLevel = FPMNum(getXpForNextLevel());
    // Player could possibly gain multiple levels at once so this has to be a loop
    while (XP >= forNextLevel) {
        // Levelup - adjust stats and gain HP/MP
        level += 1;
        XP -= forNextLevel;
        forNextLevel = FPMNum(getXpForNextLevel());
        auto additionalHP = FPMNum(LEVELUP_HP_FACTOR) * maxHP - maxHP;
        maxHP += additionalHP;
        auto additionalMP = FPMNum(LEVELUP_MP_FACTOR) * maxMP - maxMP;
        maxMP += additionalMP;
        if (!this->isDead()) {
            HP += additionalHP;
            MP += additionalMP;
        }
        attackCooldownMS *= FPMNum24(LEVELUP_ATTACK_COOLDOWN_FACTOR);
        attackDamageHP *= FPMNum(LEVELUP_ATTACK_DMG_FACTOR);
    }
}

void Player::regainMP(FPMNum amount) {
    this->MP += amount;
    if (this->MP > this->maxMP)
        this->MP = this->maxMP;
}

bool Player::updateDrawables(const sf::FloatRect& frustum, float elapsedSeconds, unsigned int elapsedMSSinceLastSimulationStep) {
    auto visible = Character::updateDrawables(frustum, elapsedSeconds, elapsedMSSinceLastSimulationStep);
    if (visible)
        nameForRendering.setPosition({sprite.getPosition().x, sprite.getPosition().y + 10.f});
    return visible;
}

void Player::drawUI(sf::RenderTarget &target) {
    Character::drawUI(target);
    target.draw(nameForRendering);
}

bool Player::canAttack(std::uint32_t targetID, bool lineOfSightCheck) {
    return characterContainer->isAlive(targetID) and canAttack(characterContainer->getCharacterByID(targetID)->getMapPosition(), lineOfSightCheck);
}

bool Player::canAttack(const FPMVector2& targetPosition, bool lineOfSightCheck) {
    if (isDead() or !tilemap->inMap(targetPosition))
        return false;
    auto a_to_t = targetPosition - mapPosition;
    auto distance = getLength(a_to_t);
    if (distance <= attackRange) {
        if (!lineOfSightCheck or distance <= FPMNum(1))
            return true;
        normalize(a_to_t);
        FPMVector2 dummy;
        return !tilemap->lineOfSightCheck(mapPosition, a_to_t, distance, dummy, dummy);
    } else
        return false;
}

bool Player::canUsePotion(POTIONS potion) {
    return !this->isDead() and ((potion == POTIONS::HP and this->numHPPotions > 0) or (potion == POTIONS::MP and this->numMPPotions > 0));
}

void Player::usePotion(POTIONS potion) {
    if (!canUsePotion(potion))
        return;

    if (potion == POTIONS::HP) {
        numHPPotions -= 1;
        heal(FPMNum(ITEM_HP_POTION_HP_GAIN));
    } else {  // MP
        numMPPotions -= 1;
        regainMP(FPMNum(ITEM_MP_POTION_MP_GAIN));
    }
}

bool Player::canBuyItem(ITEMS item) {
    return !this->isDead() and gold >= getCostOfItem(item) and tilemap->getShop().contains(this->mapPosition);
}

void Player::buyItem(ITEMS item) {
    if (!canBuyItem(item))
        return;

    gold -= getCostOfItem(item);

    // Now comes the actual effect of the bought item
    switch (item) {
        case ITEMS::HP_POTION:
            this->numHPPotions += 1;
            break;
        case ITEMS::MP_POTION:
            this->numMPPotions += 1;
            break;
        case ITEMS::HP_TOME:
            HP += FPMNum(ITEM_HP_TOME_MAX_HP_INCREASE);
            maxHP += FPMNum(ITEM_HP_TOME_MAX_HP_INCREASE);
            break;
        case ITEMS::MP_TOME:
            MP += FPMNum(ITEM_MP_TOME_MAX_MP_INCREASE);
            maxMP += FPMNum(ITEM_MP_TOME_MAX_MP_INCREASE);
            break;
        case ITEMS::XP_TOME:
            gainXp(FPMNum(ITEM_XP_TOME_XP_GAIN));
            break;
        case ITEMS::MOVEMENT_TOME:
            maxMovementPerSecond += ITEM_MOVEMENT_TOME_MOVEMENT_GAIN;
            break;
        case ITEMS::ATTACK_COOLDOWN_TOME:
            attackCooldownMS *= FPMNum24(ITEM_ATTACK_COOLDOWN_TOME_FACTOR);
            break;
        case ITEMS::ATTACK_DAMAGE_TOME:
            attackDamageHP *= FPMNum(ITEM_ATTACK_DAMAGE_TOME_FACTOR);
            break;
        case ITEMS::SKILL_COOLDOWN_TOME:
            for (auto &i : skillsCooldownMS)
                i *= FPMNum24(ITEM_SKILL_COOLDOWN_TOME_FACTOR);
            break;
        case ITEMS::NUM_ITEMS:
            throw std::runtime_error("boughtItem called with NUM_ITEMS");
    }
}

void Player::makeAOEAttack(const FPMVector2& where, FPMNum radius, FPMNum damageAmount) {
    // TODO better have a function in charactercontainer that we can give a lambda or sth
    auto nearbyCharacters = characterContainer->getCharactersAtWithTolerance(where, radius);
    for (const auto &c: *nearbyCharacters) {
        if (!c->isPlayerOrAlly() and getLength(where - c->getMapPosition()) <= radius) {
            c->setAnimationState(ANIMATION_STATE::HIT);
            c->harm(damageAmount, ID);
        }
    }
}

bool Player::canUseSkill(unsigned int skillSlot, std::uint32_t targetID, const FPMVector2& targetPosition) {
    auto skill = skillSlotToSkill(this->type, skillSlot);
    if (this->isDead() or skillsTimer[skillSlot - 1] > FPMNum24(0) or MP < getMPCostOfSkill(skill))
        return false;

    auto skillInfo = getSkillInfo(skill);
    switch (skillInfo.targetType) {
        case SKILL_TARGET_TYPES::SINGLE_CREEP:
            return canAttack(targetID, skillInfo.checkLineOfSight) and !characterContainer->getCharacterByID(targetID)->isPlayerOrAlly();
        case SKILL_TARGET_TYPES::RADIUS:
            return canAttack(targetPosition, skillInfo.checkLineOfSight);
        case SKILL_TARGET_TYPES::SINGLE_ALLY:
            return canAttack(targetID, skillInfo.checkLineOfSight) and characterContainer->getCharacterByID(targetID)->isPlayerOrAlly();
        case SKILL_TARGET_TYPES::FREE_SPOT: {
            assert(!skillInfo.checkLineOfSight);
            if (!tilemap->inMap(targetPosition))
                return false;
            bool freeSpot = true;
            if (std::any_of(tilemap->getObstaclesAt(targetPosition).begin(),
                            tilemap->getObstaclesAt(targetPosition).end(),
                            [&](const auto &o) { return o->contains(targetPosition); }) or
                std::any_of(characterContainer->getCharactersAt(targetPosition).begin(),
                            characterContainer->getCharactersAt(targetPosition).end(),
                            [&](const auto &c) {
                                return c->checkCollisionWithCircle(targetPosition, this->getGroundRadius());
                            }))
                freeSpot = false;
            return freeSpot;
        }
        default:
            return true;
    }
}

void Player::useSkill(unsigned int skillSlot, std::uint32_t targetID, const FPMVector2& targetPosition) {
    if (!canUseSkill(skillSlot, targetID, targetPosition))
        return;

    auto skill = skillSlotToSkill(this->type, skillSlot);
    MP -= getMPCostOfSkill(skill);
    skillsTimer[skillSlot - 1] = skillsCooldownMS[skillSlot - 1];

    // Now comes the actual effect of the skill
    switch (skill) {
        case SKILLS::MULTI_ARROW:
            makeAOEAttack(targetPosition, FPMNum(PLAYER_ARCHER_MULTIARROW_RADIUS), attackDamageHP);
            break;
        case SKILLS::COLD_ARROW: {
            auto targetCharacter = characterContainer->getCharacterByID(targetID);
            targetCharacter->setAnimationState(ANIMATION_STATE::HIT);
            targetCharacter->harm(attackDamageHP, ID);
            targetCharacter->giveCondition(CONDITIONS::IMMOBILE, FPMNum24((PLAYER_ARCHER_COOLARROW_SECONDS + PLAYER_ARCHER_COOLARROW_LEVELUP_EXTRA_SECONDS * (skillsLevel[skillSlot] - 1)) * 1000), ID);
        } break;
        case SKILLS::TANK:
            this->giveCondition(CONDITIONS::IMMUNE_TO_DAMAGE, FPMNum24((PLAYER_KNIGHT_TANK_SECONDS + PLAYER_KNIGHT_TANK_LEVELUP_EXTRA_SECONDS * (skillsLevel[skillSlot] - 1)) * 1000), ID);
            break;
        case SKILLS::CARNAGE:
            makeAOEAttack(mapPosition, FPMNum(PLAYER_KNIGHT_CARNAGE_RADIUS + PLAYER_KNIGHT_CARNAGE_EXTRA_RADIUS * (skillsLevel[skillSlot] - 1)), attackDamageHP);
            break;
        case SKILLS::MIGHT: {
            auto targetCharacter = characterContainer->getCharacterByID(targetID);
            targetCharacter->setAnimationState(ANIMATION_STATE::HIT);
            targetCharacter->harm(attackDamageHP * FPMNum(PLAYER_KNIGHT_MIGHT_FACTOR + PLAYER_KNIGHT_MIGHT_LEVELUP_EXTRA_FACTOR * (skillsLevel[skillSlot] - 1)), ID);
        } break;
        case SKILLS::FIREBALL: {
            auto targetCharacter = characterContainer->getCharacterByID(targetID);
            targetCharacter->setAnimationState(ANIMATION_STATE::HIT);
            targetCharacter->harm(attackDamageHP * FPMNum(PLAYER_MAGE_FIREBALL_FACTOR + PLAYER_MAGE_FIREBALL_LEVELUP_EXTRA_FACTOR * (skillsLevel[skillSlot] - 1)), ID);
        } break;
        case SKILLS::DEATHZONE_MAGE:
            zonePosition = targetPosition;
            zoneTimer = FPMNum24((PLAYER_MAGE_DEATHZONE_SECONDS + PLAYER_MAGE_DEATHZONE_LEVELUP_EXTRA_SECONDS * (skillsLevel[skillSlot] - 1)) * 1000);
            break;
        case SKILLS::ICE_BOMB: {
            // TODO reduce code dupliaction with makeAOEAttack
            auto nearbyCharacters = characterContainer->getCharactersAtWithTolerance(targetPosition, FPMNum(PLAYER_MAGE_ICEBOMB_RADIUS));
            for (const auto &c: *nearbyCharacters) {
                if (!c->isPlayerOrAlly() and getLength(targetPosition - c->getMapPosition()) <= FPMNum(PLAYER_MAGE_ICEBOMB_RADIUS)) {
                    c->setAnimationState(ANIMATION_STATE::HIT);
                    c->harm(attackDamageHP * PLAYER_MAGE_ICEBOMB_FACTOR, ID);
                    c->giveCondition(CONDITIONS::IMMOBILE, FPMNum24((PLAYER_MAGE_ICEBOMB_SECONDS + PLAYER_MAGE_ICEBOMB_EXTRA_SECONDS * (skillsLevel[skillSlot] - 1)) * 1000), ID);
                }
            }
        } break;
        case SKILLS::HEAL:
            characterContainer->getCharacterByID(targetID)->heal(FPMNum(PLAYER_MONK_HEAL_HP + PLAYER_MONK_HEAL_LEVELUP_EXTRA_HP * (skillsLevel[skillSlot] - 1)));
            this->gainXp(FPMNum(PLAYER_MONK_HEAL_XP));
            break;
        case SKILLS::DEATHZONE_MONK:
            zonePosition = targetPosition;
            zoneTimer = FPMNum24((PLAYER_MONK_DEATHZONE_SECONDS + PLAYER_MONK_DEATHZONE_LEVELUP_EXTRA_SECONDS * (skillsLevel[skillSlot] - 1)) * 1000);
            break;
        case SKILLS::TELEPORT:
            this->mapPosition = targetPosition;
            break;
        case SKILLS::CONFUSE: {
            // TODO reduce code dupliaction with makeAOEAttack
            auto nearbyCharacters = characterContainer->getCharactersAtWithTolerance(targetPosition, FPMNum(PLAYER_MAGE_CONFUSE_RADIUS));
            for (const auto &c: *nearbyCharacters) {
                if (!c->isPlayerOrAlly() and getLength(targetPosition - c->getMapPosition()) <= FPMNum(PLAYER_MAGE_CONFUSE_RADIUS)) {
                    c->setAnimationState(ANIMATION_STATE::HIT);
                    c->giveCondition(CONDITIONS::CONFUSED, FPMNum24((PLAYER_MAGE_CONFUSE_SECONDS + PLAYER_MAGE_CONFUSE_LEVELUP_EXTRA_SECONDS * (skillsLevel[skillSlot] - 1)) * 1000), ID);
                }
            }
        } break;
        case SKILLS::CREATE_SCARECROW:
            createScarecrowFlag = true;
            break;
        case SKILLS::MASS_HEAL:
            for (unsigned int targetID = 0; targetID < MAX_NUM_PLAYERS; targetID++) {
                if (!characterContainer->isAlive(targetID))
                    continue;
                characterContainer->getCharacterByID(targetID)->heal(FPMNum(PLAYER_MONK_MASS_HEAL_HP + PLAYER_MONK_MASS_HEAL_LEVELUP_EXTRA_HP * (skillsLevel[skillSlot] - 1)));
            }
            this->gainXp(FPMNum(PLAYER_MONK_MASS_HEAL_XP));
            break;
        case SKILLS::POISON_VIAL: {
            // TODO reduce code dupliaction with makeAOEAttack
            auto nearbyCharacters = characterContainer->getCharactersAtWithTolerance(targetPosition, FPMNum(PLAYER_ARCHER_POISON_VIAL_RADIUS));
            for (const auto &c: *nearbyCharacters) {
                if (!c->isPlayerOrAlly() and getLength(targetPosition - c->getMapPosition()) <= FPMNum(PLAYER_ARCHER_POISON_VIAL_RADIUS)) {
                    c->setAnimationState(ANIMATION_STATE::HIT);
                    c->giveCondition(CONDITIONS::POISONED, FPMNum24((PLAYER_ARCHER_POISON_VIAL_SECONDS + PLAYER_ARCHER_POISON_VIAL_LEVELUP_EXTRA_SECONDS * (skillsLevel[skillSlot] - 1)) * 1000), ID, FPMNum(PLAYER_ARCHER_POISON_VIAL_DMG_PER_SEC + PLAYER_ARCHER_POISON_VIAL_LEVELUP_EXTRA_DMG * (skillsLevel[skillSlot] - 1)));
                }
            }
        } break;
        case SKILLS::RAGE:
            this->giveCondition(CONDITIONS::ENRAGED, FPMNum24((PLAYER_KNIGHT_RAGE_SECONDS + PLAYER_KNIGHT_RAGE_LEVELUP_EXTRA_SECONDS * (skillsLevel[skillSlot] - 1)) * 1000), ID);
            break;
        default:
            throw std::runtime_error("Invalid skill in Player::useSkill");
    }

    if (tilesetExists(type, ANIMATION_STATE::SPELL))
        setAnimationState(ANIMATION_STATE::SPELL);
}

std::string Player::getSkillTooltipText(unsigned int skillSlot) {
    auto skill = skillSlotToSkill(this->type, skillSlot);
    switch (skill) {
        case SKILLS::USE_DISTRACTION:
            return toStr("Use distraction\n", "Attack damage * ", PLAYER_ARCHER_DISTRACTION_DMG_FACTOR + PLAYER_ARCHER_DISTRACTION_LEVELUP_DMG_FACTOR_ADD * (skillsLevel[skillSlot] - 1), " if ally near target", "\nPassive, always on");
        case SKILLS::MULTI_ARROW:
            return toStr("Multi arrow\n", "Hit targets in ", PLAYER_ARCHER_MULTIARROW_RADIUS, "m radius", "\nCost: ", static_cast<float>(getMPCostOfSkill(skill)), "MP", "\nCooldown ", getSkillCooldownSec(skillSlot), "s", "\nShortcut ", skillSlot);
        case SKILLS::COLD_ARROW:
            return toStr("Cold arrow\n", "Damage and slow target for ", PLAYER_ARCHER_COOLARROW_SECONDS + PLAYER_ARCHER_COOLARROW_LEVELUP_EXTRA_SECONDS * (skillsLevel[skillSlot] - 1), "s", "\nCost: ", static_cast<float>(getMPCostOfSkill(skill)), "MP", "\nCooldown ", getSkillCooldownSec(skillSlot), "s", "\nShortcut ", skillSlot);
        case SKILLS::TANK:
            return toStr("Tank\n", "Reduce incoming damage by 80% for ", PLAYER_KNIGHT_TANK_SECONDS + PLAYER_KNIGHT_TANK_LEVELUP_EXTRA_SECONDS * (skillsLevel[skillSlot] - 1), "s", "\nCost: ", static_cast<float>(getMPCostOfSkill(skill)), "MP", "\nCooldown ", getSkillCooldownSec(skillSlot), "s", "\nShortcut ", skillSlot);
        case SKILLS::CARNAGE:
            return toStr("Carnage\n", "Hit all targets in a\n", PLAYER_KNIGHT_CARNAGE_RADIUS + PLAYER_KNIGHT_CARNAGE_EXTRA_RADIUS * (skillsLevel[skillSlot] - 1), "m circle around you", "\nCost: ", static_cast<float>(getMPCostOfSkill(skill)), "MP", "\nCooldown ", getSkillCooldownSec(skillSlot), "s", "\nShortcut ", skillSlot);
        case SKILLS::MIGHT:
            return toStr("Might\n", "Make an attack with ", PLAYER_KNIGHT_MIGHT_FACTOR + PLAYER_KNIGHT_MIGHT_LEVELUP_EXTRA_FACTOR * (skillsLevel[skillSlot] - 1), "x damage", "\nCost: ", static_cast<float>(getMPCostOfSkill(skill)), "MP", "\nCooldown ", getSkillCooldownSec(skillSlot), "s", "\nShortcut ", skillSlot);
        case SKILLS::FIREBALL:
            return toStr("Fireball\n", "Make an attack with ", PLAYER_MAGE_FIREBALL_FACTOR + PLAYER_MAGE_FIREBALL_LEVELUP_EXTRA_FACTOR * (skillsLevel[skillSlot] - 1), "x damage", "\nCost: ", static_cast<float>(getMPCostOfSkill(skill)), "MP", "\nCooldown ", getSkillCooldownSec(skillSlot), "s", "\nShortcut ", skillSlot);
        case SKILLS::DEATHZONE_MAGE:
            return toStr("Death zone\n", "Create a ", PLAYER_MAGE_DEATHZONE_RADIUS, "m zone dealing \n", PLAYER_MAGE_DEATHZONE_FACTOR, "x damage for ", PLAYER_MAGE_DEATHZONE_SECONDS + PLAYER_MAGE_DEATHZONE_LEVELUP_EXTRA_SECONDS * (skillsLevel[skillSlot] - 1), "s", "\nCost: ", static_cast<float>(getMPCostOfSkill(skill)), "MP", "\nCooldown ", getSkillCooldownSec(skillSlot), "s", "\nShortcut ", skillSlot);
        case SKILLS::ICE_BOMB:
            return toStr("Ice bomb\n", "Make an attack with ", PLAYER_MAGE_ICEBOMB_RADIUS, "m radius\nthat deals ", PLAYER_MAGE_ICEBOMB_FACTOR, "x damage once and slows targets for ", PLAYER_MAGE_ICEBOMB_SECONDS + PLAYER_MAGE_ICEBOMB_EXTRA_SECONDS * (skillsLevel[skillSlot] - 1), "s", "\nCost: ", static_cast<float>(getMPCostOfSkill(skill)), "MP", "\nCooldown ", getSkillCooldownSec(skillSlot), "s", "\nShortcut ", skillSlot);
        case SKILLS::HEAL:
            return toStr("Heal", "Heal an ally for ", PLAYER_MONK_HEAL_HP + PLAYER_MONK_HEAL_LEVELUP_EXTRA_HP * (skillsLevel[skillSlot] - 1), "HP\nGet ", PLAYER_MONK_HEAL_XP, " XP", "\nCost: ", static_cast<float>(getMPCostOfSkill(skill)), "MP", "\nCooldown ", getSkillCooldownSec(skillSlot), "s", "\nShortcut ", skillSlot);
        case SKILLS::DEATHZONE_MONK:
            return toStr("Death zone\n", "Create a ", PLAYER_MONK_DEATHZONE_RADIUS, "m zone dealing \n", PLAYER_MONK_DEATHZONE_FACTOR, "x damage for ", PLAYER_MONK_DEATHZONE_SECONDS + PLAYER_MONK_DEATHZONE_LEVELUP_EXTRA_SECONDS * (skillsLevel[skillSlot] - 1), "s", "\nCost: ", static_cast<float>(getMPCostOfSkill(skill)), "MP", "\nCooldown ", getSkillCooldownSec(skillSlot), "s", "\nShortcut ", skillSlot);
        case SKILLS::TELEPORT:
            return toStr("Teleport\n", "Teleport to a free\nspot anywhere", "\nCost: ", static_cast<float>(getMPCostOfSkill(skill)), "MP", "\nCooldown ", getSkillCooldownSec(skillSlot), "s", "\nShortcut ", skillSlot);
        case SKILLS::CONFUSE:
            return toStr("Confuse\n", "Confuse creeps in a ", PLAYER_MAGE_CONFUSE_RADIUS, "m radius for ", PLAYER_MAGE_CONFUSE_SECONDS + PLAYER_MAGE_CONFUSE_LEVELUP_EXTRA_SECONDS * (skillsLevel[skillSlot] - 1), "s.\nThey lose their targets and move randomly\nCost: ", static_cast<float>(getMPCostOfSkill(skill)), "MP", "\nCooldown ", getSkillCooldownSec(skillSlot), "s", "\nShortcut ", skillSlot);
        case SKILLS::CREATE_SCARECROW:
            return toStr("Create scarecrow\n", "Place a scarecrow with ", PLAYER_ARCHER_CREATE_SCARECROW_HP + PLAYER_ARCHER_CREATE_SCARECROW_LEVELUP_EXTRA_HP * (skillsLevel[skillSlot] - 1), "HP\nthat attracts creeps but does not attack\nCost: ", static_cast<float>(getMPCostOfSkill(skill)), "MP", "\nCooldown ", getSkillCooldownSec(skillSlot), "s", "\nShortcut ", skillSlot);
        case SKILLS::MASS_HEAL:
            return toStr("Mass heal\n", "Heal all alive allies for ", PLAYER_MONK_MASS_HEAL_HP + PLAYER_MONK_MASS_HEAL_LEVELUP_EXTRA_HP * (skillsLevel[skillSlot] - 1), "HP\nGet ", PLAYER_MONK_MASS_HEAL_XP, "XP\nCost: ", static_cast<float>(getMPCostOfSkill(skill)), "MP", "\nCooldown ", getSkillCooldownSec(skillSlot), "s", "\nShortcut ", skillSlot);
        case SKILLS::POISON_VIAL:
            return toStr("Poison vial\n", "Poison creeps in a ", PLAYER_ARCHER_POISON_VIAL_RADIUS, "m radius.\nThey take ", PLAYER_ARCHER_POISON_VIAL_DMG_PER_SEC + PLAYER_ARCHER_POISON_VIAL_LEVELUP_EXTRA_DMG * (skillsLevel[skillSlot] - 1), " damage per second for ", PLAYER_ARCHER_POISON_VIAL_SECONDS + PLAYER_ARCHER_POISON_VIAL_LEVELUP_EXTRA_SECONDS * (skillsLevel[skillSlot] - 1), "s", "\nCost: ", static_cast<float>(getMPCostOfSkill(skill)), "MP", "\nCooldown ", getSkillCooldownSec(skillSlot), "s", "\nShortcut ", skillSlot);
        case SKILLS::RAGE:
            return toStr("Rage\n", "Reduce your attack cooldown by ", PLAYER_KNIGHT_RAGE_ATTACK_COOLDOWN_FACTOR, "x for ", PLAYER_KNIGHT_RAGE_SECONDS + PLAYER_KNIGHT_RAGE_LEVELUP_EXTRA_SECONDS * (skillsLevel[skillSlot] - 1), "s\nCost: ", static_cast<float>(getMPCostOfSkill(skill)), "MP", "\nCooldown ", getSkillCooldownSec(skillSlot), "s", "\nShortcut ", skillSlot);
        default:
            throw std::runtime_error("NUM_SKILLS has no tooltip");
    }
}

FPMNum Player::getZoneRadius() const {
    switch (this->type) {
        case CHARACTERS::MAGE:
            return FPMNum(PLAYER_MAGE_DEATHZONE_RADIUS);
        case CHARACTERS::MONK:
            return FPMNum(PLAYER_MONK_DEATHZONE_RADIUS);
        default:
            throw std::runtime_error("This player does not have a death zone");
    }
}

bool Player::gameShouldCreateScarecrow() {
    if (createScarecrowFlag) {
        createScarecrowFlag = false;
        return true;
    } else
        return false;
}

bool Player::canUpgradeSkill(unsigned int skillSlot) {
    // At each level up, one skill can be upgraded. So we need to make sure that 1. the current skill level is not
    // already the maximum level and 2. the player hasn't already spent all the level-ups gained so far.
    return skillsLevel[skillSlot] < getMaxSkillLevel(skillSlotToSkill(this->type, skillSlot)) and
           std::accumulate(skillsLevel.begin(), skillsLevel.end(), 0) - 6 < level;
}

void Player::upgradeSkill(unsigned int skillSlot) {
    if (!canUpgradeSkill(skillSlot))
        return;
    skillsLevel[skillSlot]++;
}
