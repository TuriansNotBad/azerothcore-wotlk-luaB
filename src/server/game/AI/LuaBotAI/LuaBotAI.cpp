
#include "LuaBotAI.h"
#include "Player.h"

LuaBotAI::LuaBotAI(Player* me, Player* master, int logicID)
    : me(me), logicID(logicID), master(master), ceaseUpdates(false)
{
    m_updateTimer.Reset(2000);
}

LuaBotAI::~LuaBotAI() {

}


void LuaBotAI::Update(uint32 diff) {
    
    // were instructed not to update; likely caused by lua error
    if (ceaseUpdates) return;
    // bad pointers
    if (!me || !master) return;
    // hardcoded cease all logic ID
    if (logicID == -1) return;
    // we're not available for interactions
    if (!me->IsInWorld() || me->IsBeingTeleported() || me->isBeingLoaded())
        return;
    // master not available, do not update
    if (!master->IsInWorld() || master->IsBeingTeleported() || master->isBeingLoaded())
        return;
    // do not gain XP
    me->SetUInt32Value(PLAYER_XP, 0);
    // master in taxi?
    if (master->HasUnitState(UNIT_STATE_IN_FLIGHT)) {
        if (me->GetMotionMaster()->GetCurrentMovementGeneratorType()) {
            me->GetMotionMaster()->Clear(true);
            me->GetMotionMaster()->MoveIdle();
        }
        return;
    }
    // follow leader
    if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() != MovementGeneratorType::FOLLOW_MOTION_TYPE) {
        me->GetMotionMaster()->Clear(true);
        me->GetMotionMaster()->MoveFollow(master, 5, 0, MOTION_SLOT_ACTIVE);
    }

}

