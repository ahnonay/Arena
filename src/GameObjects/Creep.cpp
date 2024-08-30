#include "Creep.h"
#include <fpm/ios.hpp>
#include <algorithm>

Creep::Creep(sf::Uint32 ID, unsigned int level, FPMVector2 spawnPosition, const std::shared_ptr<Tilemap> &tilemap, const std::shared_ptr<CharacterContainer> &characterContainer, unsigned int randomSeed)
        : Character(ID, levelToCharacterType(level), spawnPosition, tilemap, characterContainer, randomSeed), wanderAngle(FPMNum(0)),
          seekTargetID(ID), seekRange(DEFAULT_CREEP_SEEK_RANGE),
          stuckTimer(0), damageReceived() {
    // Set creep stats according to the defaults in Constant.h and the current crep level
    std::uniform_int_distribution<int> dist(DEFAULT_CREEP_MAX_MOVEMENT_PER_SEC * 1000 - 500, DEFAULT_CREEP_MAX_MOVEMENT_PER_SEC * 1000 + 500);
    maxMovementPerSecond = FPMNum(dist(gen)) / FPMNum(1000);

    maxHP = FPMNum(CREEP_INITIAL_HP) * fpm::pow(FPMNum(LEVELUP_HP_FACTOR), FPMNum(level) - 1);
    attackCooldownMS = FPMNum24(CREEP_INITIAL_ATTACK_COOLDOWN_MS) * fpm::pow(FPMNum24(LEVELUP_ATTACK_COOLDOWN_FACTOR), FPMNum24(level) - 1);
    attackDamageHP = FPMNum(CREEP_INITIAL_ATTACK_DMG) * fpm::pow(FPMNum(LEVELUP_ATTACK_DMG_FACTOR), FPMNum(level) - 1);

    // We also have some special levels, e.g. with extra fast or strong creeps
    if (level == 3) {
        assert(type == CHARACTERS::OGRE);
        attackCooldownMS *= FPMNum24(1.5);
        maxHP *= FPMNum(1.5);
    } else if (level == 7) {
        assert(type == CHARACTERS::SPIDER);
        maxMovementPerSecond *= 2;
        maxHP /= 3;
    } else if (level == 10) {
        assert(type == CHARACTERS::CROCODILE);
        attackDamageHP *= 2;
        maxHP /= FPMNum(1.5);
    } else if (level == 13) {
        assert(type == CHARACTERS::BAT);
        maxMovementPerSecond *= FPMNum(2.5);
        maxHP /= 2;
    } else if (level == 16) {
        assert(type == CHARACTERS::DARK_DWARF);
        maxMovementPerSecond /= 3;
        maxHP *= 3;
    } else if (level == 18) {
        assert(type == CHARACTERS::PINK_ZOMBIE);
        attackDamageHP /= 3;
        maxHP *= 4;
    }

    HP = maxHP;
    damageReceived.fill(FPMNum(0));
}

CHARACTERS Creep::levelToCharacterType(unsigned int creepLevel) {
    switch (creepLevel) {
        case 1:
            return CHARACTERS::WOLF;
        case 2:
            return CHARACTERS::GNOME;
        case 3:
            return CHARACTERS::OGRE;
        case 4:
            return CHARACTERS::GHOST;
        case 5:
            return CHARACTERS::ZOMBIE;
        case 6:
            return CHARACTERS::ORC;
        case 7:
            return CHARACTERS::SPIDER;
        case 8:
            return CHARACTERS::DINO;
        case 9:
            return CHARACTERS::BLUE_KNIGHT;
        case 10:
            return CHARACTERS::CROCODILE;
        case 11:
            return CHARACTERS::GREEN_ZOMBIE;
        case 12:
            return CHARACTERS::GREEN_DWARF;
        case 13:
            return CHARACTERS::BAT;
        case 14:
            return CHARACTERS::RED_KNIGHT;
        case 15:
            return CHARACTERS::GREEN_DINO;
        case 16:
            return CHARACTERS::DARK_DWARF;
        case 17:
            return CHARACTERS::GREEN_KNIGHT;
        case 18:
            return CHARACTERS::PINK_ZOMBIE;
        default:
            return CHARACTERS::BLACK_KNIGHT;  // The final character
    }
}

void Creep::harm(FPMNum amountHP, sf::Uint32 attackerID) {
    if (this->HP <= FPMNum(0))
        return;

    // Keep track of who applied how much damage to this creep
    auto dmg = std::min(amountHP, HP);
    if (attackerID >= MAX_NUM_PLAYERS)
        attackerID = MAX_NUM_PLAYERS;
    damageReceived[attackerID] += dmg;

    Character::harm(amountHP, attackerID);

    // If the creep just died, inform all its attackers so they can gain XP etc.
    if (this->HP <= FPMNum(0)) { // just died
        FPMNum totalDamage(0);
        for (auto d : damageReceived)
            totalDamage += d;
        for (int i = 0; i < MAX_NUM_PLAYERS; i++) {
            if (damageReceived[i] > FPMNum(0))
                characterContainer->getCharacterByID(i)->killedCreep(damageReceived[i] / totalDamage);
        }
    }
}

bool Creep::hasReachedGoal() const {
    return tilemap->getCreepGoal().contains(mapPosition);
}

void Creep::simulate() {
    if (this->isDead())
        throw std::runtime_error("Trying to simulate dead creep!");
    if (this->hasReachedGoal())
        throw std::runtime_error("Trying to simulate creep that reached goal!");

    /////////////////////////////////////////////////////////////////
    /// Determine creep movement and action
    /////////////////////////////////////////////////////////////////
    // If creep is currently attacking player, continue as long as the player is in range
    if (attackTargetID != this->ID) {
        if (!characterContainer->isAlive(attackTargetID) or
            this->hasCondition(CONDITIONS::IMMOBILE) or
            this->hasCondition(CONDITIONS::CONFUSED) or
            getLengthSq(characterContainer->getCharacterByID(attackTargetID)->getMapPosition() - mapPosition) > attackRange * attackRange)
            attackTargetID = this->ID;
    }
    if (attackTargetID == this->ID) {
        // Currently, creep has no target. However:
        // If a player is in range, attack them. If no player is in range, but they are inside the seek range, seek them
        // In case of multiple potential targets, choose at random
        if (seekTargetID != this->ID) { // First, check if we lost the current seek target
            if (!characterContainer->isAlive(seekTargetID) or
                this->hasCondition(CONDITIONS::IMMOBILE) or
                this->hasCondition(CONDITIONS::CONFUSED) or
                getLengthSq(characterContainer->getCharacterByID(seekTargetID)->getMapPosition() - mapPosition) > seekRange * seekRange)
                seekTargetID = this->ID;
        }
        if (!this->hasCondition(CONDITIONS::CONFUSED) and !this->hasCondition(CONDITIONS::IMMOBILE)) {
            std::vector<unsigned int> targetsInAttackRange, targetsInSeekRange;
            auto nearbyCharacters = characterContainer->getCharactersAtWithTolerance(mapPosition, std::max(attackRange, seekRange));
            for (const auto &c: *nearbyCharacters) {
                if (!c->isPlayerOrAlly() or c->isDead())
                    continue;
                FPMNum distSqToPlayer = getLengthSq(c->getMapPosition() - mapPosition);
                if (distSqToPlayer <= attackRange * attackRange)
                    targetsInAttackRange.push_back(c->getID());
                else if (seekTargetID == this->ID and distSqToPlayer <= seekRange * seekRange)
                    targetsInSeekRange.push_back(c->getID());
            }
            if (!targetsInAttackRange.empty()) {
                std::uniform_int_distribution<int> dist(0, targetsInAttackRange.size() - 1);
                attackTargetID = targetsInAttackRange[dist(gen)];
            } else if (!targetsInSeekRange.empty()) {
                std::uniform_int_distribution<int> dist(0, targetsInSeekRange.size() - 1);
                seekTargetID = targetsInSeekRange[dist(gen)];
            }
        }

        // Now, determine movement
        FPMVector2 desiredVelocity;
        if (this->hasCondition(CONDITIONS::IMMOBILE))
            setNull(desiredVelocity);
        else if (stuckTimer * SIMULATION_TIME_STEP_MS >= FPMNum(CREEP_STUCK_TIMER_MS)) {
            // If the creep has been stuck for some time, just wander randomly
            wanderVelocity = wander();
            desiredVelocity = wanderVelocity;
            setNull(flowfieldVelocity);
            setNull(seekVelocity);
            setNull(separationVelocity);
            setNull(obstaclesVelocity);
        } else if (this->hasCondition(CONDITIONS::CONFUSED)) {
            wanderVelocity = wander();
            separationVelocity = separation();
            obstaclesVelocity = avoidObstacles();
            desiredVelocity = wanderVelocity + FPMNum(3) * separationVelocity + FPMNum(3) * obstaclesVelocity;
            setNull(flowfieldVelocity);
            setNull(seekVelocity);
        } else {
            // If creep is currently seeking a player, ignore flowfield
            // Otherwise, follow flowfield while also introducing some randomness through wander()
            if (seekTargetID != this->ID) {
                seekVelocity = seek(characterContainer->getCharacterByID(seekTargetID)->getMapPosition());
                setNull(flowfieldVelocity);
                setNull(wanderVelocity);
            }
            else {
                setNull(seekVelocity);
                flowfieldVelocity = followFlowfield();
                wanderVelocity = wander();
            }
            // Always keep away from other creeps and walls
            separationVelocity = separation();
            obstaclesVelocity = avoidObstacles();
            desiredVelocity = FPMNum(2) * seekVelocity + FPMNum(2) * flowfieldVelocity + wanderVelocity +
                    FPMNum(3) * separationVelocity + FPMNum(3) * obstaclesVelocity;
        }

        // Normally, for steering behaviors we would do:
        // auto steering = desiredVelocity - velocity;
        // auto mass = FPMNum(0.1); // <0 makes character more responsive
        // steering /= mass;
        // clipLength(steering, maxForcePerSecond);
        // velocity += steering * FPMNum(SIMULATION_TIME_STEP_SEC);
        // But instead, we do instant movement (zero inertia), because it seems more realistic for our characters:
        velocity = desiredVelocity;
        clipLength(velocity, maxMovementPerSecond, FPMNum(0.01));

        // Finally, apply the movement and update the scene graph
        FPMVector2 newPosition;
        if (getNextSimulationPosition(newPosition)) {
            stuckTimer = FPMNum(0);
            if (tilemap->getCreepGoal().contains(newPosition))
                characterContainer->remove(this, mapPosition, groundRadius);
            else
                characterContainer->update(this, mapPosition, newPosition, groundRadius);
            mapPosition = newPosition;
        } else {
            setNull(velocity);
            stuckTimer += FPMNum(1);
        }
    } else {
        // Don't move in this step, just keep attacking
        setNull(velocity);
        setNull(seekVelocity);
        setNull(flowfieldVelocity);
        setNull(wanderVelocity);
        setNull(separationVelocity);
        setNull(obstaclesVelocity);
        if (attackTimer <= FPMNum24(0)) {
            attackTimer = getAttackCooldownMS();
            characterContainer->getCharacterByID(attackTargetID)->harm(attackDamageHP, ID);
        } else
            attackTimer -= SIMULATION_TIME_STEP_MS;
    }

    Character::simulate();
}

void Creep::drawUI(sf::RenderTarget &target) {
    Character::drawUI(target);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::V)) { // just for debugging
        auto worldPosition = tilemap->mapToWorld(mapPosition);
        sf::Vertex line[] = {sf::Vertex(worldPosition, sf::Color::Red),
                             sf::Vertex(tilemap->mapToWorld(mapPosition + velocity), sf::Color::Red)};
        target.draw(line, 2, sf::Lines);
        sf::Vertex line2[] = {sf::Vertex(worldPosition, sf::Color::Green),
                              sf::Vertex(tilemap->mapToWorld(mapPosition + seekVelocity), sf::Color::Green)};
        target.draw(line2, 2, sf::Lines);
        sf::Vertex line3[] = {sf::Vertex(worldPosition, sf::Color::Blue),
                              sf::Vertex(tilemap->mapToWorld(mapPosition + wanderVelocity), sf::Color::Blue)};
        target.draw(line3, 2, sf::Lines);
        sf::Vertex line4[] = {sf::Vertex(worldPosition, sf::Color::Yellow),
                              sf::Vertex(tilemap->mapToWorld(mapPosition + flowfieldVelocity), sf::Color::Yellow)};
        target.draw(line4, 2, sf::Lines);
        sf::Vertex line5[] = {sf::Vertex(worldPosition, sf::Color::Magenta),
                              sf::Vertex(tilemap->mapToWorld(mapPosition + separationVelocity), sf::Color::Magenta)};
        target.draw(line5, 2, sf::Lines);
        sf::Vertex line6[] = {sf::Vertex(worldPosition, sf::Color::Cyan),
                              sf::Vertex(tilemap->mapToWorld(mapPosition + obstaclesVelocity), sf::Color::Cyan)};
        target.draw(line6, 2, sf::Lines);
    }
}

FPMVector2 Creep::seek(const FPMVector2 &target) {
    auto desiredVelocity = target - mapPosition;
    setLength(desiredVelocity, maxMovementPerSecond);
    return desiredVelocity;
}

FPMVector2 Creep::flee(const FPMVector2 &target) {
    return -seek(target);
}

FPMVector2 Creep::arrive(const FPMVector2 &target, FPMNum slowingDistance, FPMNum tolerance) {
    auto desiredVelocity = target - mapPosition;
    FPMNum distance = getLength(desiredVelocity);

    if (distance > slowingDistance)
        setLength(desiredVelocity, maxMovementPerSecond);
    else if (distance > tolerance)
        setLength(desiredVelocity, maxMovementPerSecond * ((distance - tolerance) / (slowingDistance - tolerance)));
    else
        setNull(desiredVelocity);

    return desiredVelocity;
}

FPMVector2 Creep::pursue(const Character &target) {
    FPMNum distance = getLength(target.getMapPosition() - mapPosition);
    auto t = distance / target.getMaxMovementPerSecond();
    return seek(target.getMapPosition() + target.getVelocity() * t); // pursue = seek future position
}

FPMVector2 Creep::evade(const Character &target) {
    return -pursue(target);
}

FPMVector2 Creep::followFlowfield() {
    auto desiredVelocity = getDirectionVector(tilemap->getFlowAt(mapPosition));
    setLength(desiredVelocity, maxMovementPerSecond);
    return desiredVelocity;
}

#define WANDER_CIRCLERADIUS FPMNum(1.f) * groundRadius
#define WANDER_CHANGE FPMNum(1.f)
#define WANDER_OFFSET FPMNum(2.1f) * groundRadius

FPMVector2 Creep::wander() {
    auto curDirection = velocity;
    normalize(curDirection);
    FPMVector2 wanderSphereCenter = mapPosition + curDirection * WANDER_OFFSET;

    std::uniform_int_distribution<int> dist(0, 1000);
    if (isNull(curDirection))
        wanderAngle = FPMNum(dist(gen)) / FPMNum(1000) * fpm::fixed_16_16::two_pi();
    else
        wanderAngle += FPMNum(dist(gen) - dist(gen)) / FPMNum(1000) * WANDER_CHANGE;

    return seek(wanderSphereCenter + fromAngle(wanderAngle) * WANDER_CIRCLERADIUS);
}

#define SEPARATION_THRESHOLD_SQ FPMNum(2)

FPMVector2 Creep::separation() {
    auto curDirection = velocity;
    normalize(curDirection);

    FPMVector2 separation;
    unsigned int separationCounter = 0;
    auto flock = characterContainer->getCharactersAtWithTolerance(mapPosition, FPMNum(1));
    for (const auto& target : *flock) {
        if (target == this)
            continue;
        auto toTarget= target->getMapPosition() - mapPosition;
        if (dotProduct(toTarget, curDirection) < FPMNum(0))
            continue;

        auto distSq = getLengthSq(toTarget);
        if (distSq < SEPARATION_THRESHOLD_SQ) {
            normalize(toTarget);
            separation += -toTarget * maxMovementPerSecond * (SEPARATION_THRESHOLD_SQ - distSq) / SEPARATION_THRESHOLD_SQ;
            separationCounter++;
        }
    }

    if (separationCounter > 0) {
        separation /= FPMNum(separationCounter);
        clipLength(separation, maxMovementPerSecond);
        return separation;
    } else
        return {FPMNum(0), FPMNum(0)};
}

FPMVector2 Creep::flocking() {
    // Combines separation, cohesion, and alignment
    auto curDirection = velocity;
    normalize(curDirection);

    FPMVector2 averageVelocity = velocity;
    FPMVector2 averagePosition;
    FPMVector2 separation;
    unsigned int counter = 0;
    unsigned int separationCounter = 0;
    auto flock = characterContainer->getCharactersAtWithTolerance(mapPosition, FPMNum(1));
    for (const auto& target : *flock) {
        if (target == this)
            continue;
        auto toTarget= target->getMapPosition() - mapPosition;
//#define FLOCKING_MAX_DISTANCE_SQ FPMNum(9)
//        if (distSq > FLOCKING_MAX_DISTANCE_SQ)
//            continue;
        if (dotProduct(toTarget, curDirection) < FPMNum(0))
            continue;

        auto distSq = getLengthSq(toTarget);
        if (distSq < SEPARATION_THRESHOLD_SQ) {
            normalize(toTarget);
            separation += -toTarget * maxMovementPerSecond * (SEPARATION_THRESHOLD_SQ - distSq) / SEPARATION_THRESHOLD_SQ;
            separationCounter++;
        }
        averageVelocity += target->getVelocity();
        averagePosition += target->getMapPosition();
        counter++;
    }

    if (counter > 0) {
        averageVelocity /= FPMNum(counter);
        averagePosition /= FPMNum(counter);
        if (separationCounter > 0)
            separation /= FPMNum(separationCounter);
        FPMVector2 cohesion = seek(averagePosition);
        FPMVector2 alignment = averageVelocity;
        clipLength(separation, maxMovementPerSecond);
        clipLength(cohesion, maxMovementPerSecond);
        clipLength(alignment, maxMovementPerSecond);
        return separation * FPMNum(0.5f) + cohesion * FPMNum(0.3f) + alignment * FPMNum(0.2f);
    } else
        return {FPMNum(0), FPMNum(0)};
}

#define AVOID_OBSTACLES_CHECK_LENGTH FPMNum(1.5f)
#define AVOID_OBSTACLES_DISTANCE FPMNum(0.5f)

FPMVector2 Creep::avoidObstacles() {
    auto curDirection = velocity;
    normalize(curDirection);

    FPMVector2 collisionPosition;
    FPMVector2 collisionNormal;
    bool collision = tilemap->lineOfSightCheck(mapPosition,curDirection,AVOID_OBSTACLES_CHECK_LENGTH,collisionPosition,collisionNormal);

    if (collision) {
        setLength(collisionNormal, AVOID_OBSTACLES_DISTANCE);
        return seek(collisionPosition + collisionNormal);
    } else
        return {FPMNum(0), FPMNum(0)};
}

//
//#define AVOIDO_CHECK_LENGTH 2.5f
//
//Vector2D
//Character::avoidCircleObstacles(const CircleObstacleContainer *obstacles) {
//    Vector2D out;
//    Vector2D forward(Velocity);
//    forward.normalise();
//    Vector2D ray(forward * AVOIDO_CHECK_LENGTH);
//    for (auto obs : *obstacles) {
//        auto diff = obs->Position - Position;
//        float diffDot = diff.dotProduct(forward);
//        if (diffDot > 0) {
//            Vector2D projection(forward * diffDot);
//            float dist = (projection - diff).length();
//
//            if (dist < obs->Radius +
//                       Radius /* this Caracter's radius + the objects radius  */
//                && projection.lengthSq() < ray.lengthSq()) {
//                Vector2D targetVelocity = forward * MaxSpeed;
//                Vector2D Perpendicular(-diff.y, diff.x);
//                targetVelocity.rotate(
//                        (Perpendicular.dotProduct(Velocity) < 0 ? -1 : 1) * 90 *
//                        DEG_TO_RAD); // determines, whether the vector is to the right or
//                // left of the other vector
//
//                out = targetVelocity - Velocity;
//                // out *= 1-projection.length()/ray.length();
//                out.truncate(MaxAcceleration);
//                // out *= 1-projection.length()/ray.length();
//
//                /*
//                velocity.add(force); //change the velocity
//                velocity.multiply(projection.length / ray.length); //and scale again
//                */
//            }
//        }
//    }
//    return out;
//}
//
//#define AVOIDC_CHECK_LENGTH 2.f
//
//Vector2D Character::avoidCharacters(const CharacterContainer *Characters) {
//    Vector2D forward(Velocity);
//    forward.normalise();
//    Vector2D ray(forward * AVOIDC_CHECK_LENGTH);
//    Vector2D out;
//    for (auto i : (*Characters)) { // for each vehicle
//        if (i == this)
//            continue;
//        Vector2D check(i->GetVelocity());
//        check.setLength(AVOIDC_CHECK_LENGTH);
//        auto diff = (i->GetPosition() + check) - Position;
//        float dotProd = diff.dotProduct(forward);
//        if (dotProd > 0) {                           // they may meet in the future
//            auto projection = forward * dotProd;       // project the forward vector
//            float dist = (projection - diff).length(); // get the distance
//
//            if (dist < i->Radius + Radius &&
//                projection.lengthSq() < ray.lengthSq()) { // they will meet in the
//                // future, we need to fix it
//                Vector2D targetVelocity = forward * MaxSpeed;
//                Vector2D Perpendicular(-diff.y, diff.x);
//                targetVelocity.rotate(
//                        (Perpendicular.dotProduct(Velocity) < 0 ? -1 : 1) * 45 *
//                        DEG_TO_RAD); // determines, whether the vector is to the right or
//                // left of the other vector
//
//                out = targetVelocity - Velocity;
//                // out *= 1-projection.length()/ray.length();
//                out.truncate(MaxAcceleration);
//            }
//        }
//    }
//    return out;
//}
//
//#define PATH_NEXTWAYPOINT_DISTANCE_SQ 1.f
//
//Vector2D Character::followPath() {
//    Vector2D out;
//    auto ln = CurrentPath.size();
//    if (ln == 0)
//        return out;
//
//    if (ln == 1)
//        out = arrive(*CurrentPath.front(), 0.1f);
//    else
//        out = seek(*CurrentPath.front());
//
//    auto distSq = (*CurrentPath.front() - Position).lengthSq();
//    if (distSq < PATH_NEXTWAYPOINT_DISTANCE_SQ) {
//        delete CurrentPath.front();
//        CurrentPath.pop();
//    }
//
//    return out;
//}
//
//
