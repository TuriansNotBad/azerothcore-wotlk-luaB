/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ACORE_LUABMOVEMENTGENERATOR_H
#define ACORE_LUABMOVEMENTGENERATOR_H

#include "FollowerReference.h"
#include "MovementGenerator.h"
#include "Optional.h"
#include "PathGenerator.h"
#include "Timer.h"
#include "Unit.h"
#include "TargetedMovementGenerator.h"

//class TargetedMovementGeneratorBase
//{
//public:
//    TargetedMovementGeneratorBase(Unit* target) { i_target.link(target, this); }
//    void stopFollowing() { }
//protected:
//    FollowerReference i_target;
//};

template<class T>
class BotChaseMovementGenerator : public MovementGeneratorMedium<T, BotChaseMovementGenerator<T>>, public TargetedMovementGeneratorBase
{
public:
    BotChaseMovementGenerator(Unit* target, ChaseRange range, ChaseAngle angle)
        : TargetedMovementGeneratorBase(target), i_path(nullptr), i_recheckDistance(0), i_recalculateTravel(true), _range(range), _angle(angle) {}
    ~BotChaseMovementGenerator() { }

    MovementGeneratorType GetMovementGeneratorType() { return CHASE_MOTION_TYPE; }

    bool DoUpdate(T*, uint32);
    void DoInitialize(T*);
    void DoFinalize(T*);
    void DoReset(T*);
    void MovementInform(T*);
    const Optional<ChaseRange> * GetRange() { return &_range; }
    const Optional<ChaseAngle> * GetAngle() { return &_angle; }
    void SetRange(float minR, float minT, float maxR, float maxT)
    {
        ChaseRange range(minR, minT, maxT, maxR);
        _range = Optional<ChaseRange>(range);
    }
    void SetAngle(float angle, float tol)
    {
        ChaseAngle ang(angle, tol);
        _angle = Optional<ChaseAngle>(ang);
    }


    bool PositionOkay(T* owner, Unit* target, Optional<float> minDistance, Optional<float> maxDistance, Optional<ChaseAngle> angle);

    void unitSpeedChanged() { _lastTargetPosition.reset(); }
    Unit* GetTarget() const { return i_target.getTarget(); }

    bool EnableWalking() const { return false; }
    bool HasLostTarget(Unit* unit) const { return unit->GetVictim() != this->GetTarget(); }

private:
    std::unique_ptr<PathGenerator> i_path;
    TimeTrackerSmall i_recheckDistance;
    bool i_recalculateTravel;

    Optional<Position> _lastTargetPosition;
    Optional<ChaseRange> _range;
    Optional<ChaseAngle> _angle;
    bool _movingTowards = true;
    bool _mutualChase = true;
};

template<class T>
class BotFollowMovementGenerator : public MovementGeneratorMedium<T, BotFollowMovementGenerator<T>>, public TargetedMovementGeneratorBase
{
public:
    BotFollowMovementGenerator(Unit* target, float range, ChaseAngle angle)
        : TargetedMovementGeneratorBase(target), i_path(nullptr), i_recheckPredictedDistanceTimer(0), i_recheckPredictedDistance(false), _range(range), _angle(angle) {}
    ~BotFollowMovementGenerator() { }

    MovementGeneratorType GetMovementGeneratorType() { return FOLLOW_MOTION_TYPE; }

    bool DoUpdate(T*, uint32);
    void DoInitialize(T*);
    void DoFinalize(T*);
    void DoReset(T*);
    void MovementInform(T*);

    Unit* GetTarget() const { return i_target.getTarget(); }

    void unitSpeedChanged() { _lastTargetPosition.reset(); }

    bool PositionOkay(Unit* target, bool isPlayerPet, bool& targetIsMoving, uint32 diff);

    static void _clearUnitStateMove(T* u) { u->ClearUnitState(UNIT_STATE_FOLLOW_MOVE); }
    static void _addUnitStateMove(T* u) { u->AddUnitState(UNIT_STATE_FOLLOW_MOVE); }

    float GetFollowRange() const { return _range; }

private:
    std::unique_ptr<PathGenerator> i_path;
    TimeTrackerSmall i_recheckPredictedDistanceTimer;
    bool i_recheckPredictedDistance;

    Optional<Position> _lastTargetPosition;
    Optional<Position> _lastPredictedPosition;
    float _range;
    ChaseAngle _angle;
};

#endif
