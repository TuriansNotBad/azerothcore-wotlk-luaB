// just checking
#include "LuaBotAI.h"
#include "LuaLibPlayer.h"
#include "Player.h"
#include "Group.h"
#include "LuaBotManager.h"
#include "lua.hpp"

const char* LuaBotAI::MTNAME = "LuaObject.AI";

LuaBotAI::LuaBotAI(Player* me, Player* master, int logicID) :
    me(me),
    master(master),

    ceaseUpdates(false),
    m_updateInterval(50),

    userDataRef(LUA_NOREF),
    userDataPlayerRef(LUA_NOREF),
    userTblRef(LUA_NOREF),

    roleID(0),
    logicID(logicID),
    logicManager(logicID),
    topGoal(-1, 0, Goal::NOPARAMS, nullptr, nullptr, nullptr)
{
    m_updateTimer.Reset(2000);
    L = sLuaBotMgr.Lua();
    topGoal.SetTerminated(true);
}

LuaBotAI::~LuaBotAI() {
    Unref(L);
}


Goal* LuaBotAI::AddTopGoal(int goalId, double life, std::vector<GoalParamP>& goalParams, lua_State* L) {
    //topGoal.Unref(L);
    //topGoal.~Goal();
    topGoal = Goal(goalId, life, goalParams, &goalManager, L);
    goalManager.ClearActivationStack(); // top goal owns all of the goals, can nuke the entire manager
    goalManager.PushGoalOnActivationStack(&topGoal);
    return &topGoal;
}


void LuaBotAI::Init() {

    if (userDataRef == LUA_NOREF)
        CreateUD(L);
    if (userDataPlayerRef == LUA_NOREF)
        CreatePlayerUD(L);
    if (userTblRef == LUA_NOREF)
        CreateUserTbl();

    if (me->getLevel() != master->getLevel()) {
        me->GiveLevel(master->getLevel());
    }

    logicManager.Init(L, this);

}


void LuaBotAI::Update(uint32 diff) {
    
    // were instructed not to update; likely caused by lua error
    if (ceaseUpdates) return;

    // Is it time to update
    m_updateTimer.Update(diff);
    if (m_updateTimer.Passed())
        m_updateTimer.Reset(m_updateInterval);
    else
        return;

    // Did we corrupt the registry
    if (userDataRef == userDataPlayerRef || userDataRef == userTblRef || userTblRef == userDataPlayerRef) {
        LOG_ERROR("luabots", "LuaAI Core: Lua registry error... [{}, {}, {}]. Ceasing.\n", userDataRef, userDataPlayerRef, userTblRef);
        ceaseUpdates = true;
        return;
    }

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

    // leave my group if master is not in it
    if (Group* g = me->GetGroup()) {
        if (!g->IsMember(master->GetGUID())) {
            g->Disband();
        }
    }

    // join group if invite
    if (me->GetGroupInvite()) {
        Group* group = me->GetGroupInvite();
        if (group->GetMembersCount() == 0)
            group->AddMember(group->GetLeader());
        group->AddMember(me);
        // group->SetLootMethod( LootMethod::GROUP_LOOT );
    }


    // let's gooo
    logicManager.Execute(L, this);
    if (!topGoal.GetTerminated()) {
        goalManager.Activate(L, this);
        goalManager.Update(L, this);
        goalManager.Terminate(L, this);
    }

    // one of the managers called error state
    if (ceaseUpdates) {
        goalManager = GoalManager();
        topGoal = Goal(0, 10.0, Goal::NOPARAMS, &goalManager, nullptr); // delete all goal objects
        topGoal.SetTerminated(true);
        // Reset AI as well to stop moving attacking, make it obivous there's an error...
        // Whisper master?
    }



}

// USER TABLE

void LuaBotAI::CreateUserTbl() {
    if (userTblRef == LUA_NOREF) {
        lua_newtable(L);
        userTblRef = luaL_ref(L, LUA_REGISTRYINDEX);
    }
}


void LuaBotAI::UnrefUserTbl(lua_State* L) {
    // unref already makes these checks but can't hurt.
    if (userTblRef != LUA_NOREF && userTblRef != LUA_REFNIL) {
        luaL_unref(L, LUA_REGISTRYINDEX, userTblRef);
        userTblRef = LUA_NOREF; // old ref no longer valid
    }
}

// AI USERDATA

void LuaBotAI::CreateUD(lua_State* L) {
    // create userdata on top of the stack pointing to a pointer of an AI object
    LuaBotAI** aiud = static_cast<LuaBotAI**>(lua_newuserdatauv(L, sizeof(LuaBotAI*), 0));
    *aiud = this; // swap the AI object being pointed to to the current instance
    luaL_setmetatable(L, MTNAME);
    // save this userdata in the registry table.
    userDataRef = luaL_ref(L, LUA_REGISTRYINDEX); // pops
}

void LuaBotAI::PushUD(lua_State* L) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, userDataRef);
}

void LuaBotAI::Unref(lua_State* L) {
    // unref already makes these checks but can't hurt.
    if (userDataRef != LUA_NOREF && userDataRef != LUA_REFNIL) {
        luaL_unref(L, LUA_REGISTRYINDEX, userDataRef);
        userDataRef = LUA_NOREF; // old ref no longer valid
    }
}

// PLAYER USERDATA

void LuaBotAI::CreatePlayerUD(lua_State* L) {
    LuaBindsAI::Player_CreateUD(me, L);
    // save this userdata in the registry table.
    userDataPlayerRef = luaL_ref(L, LUA_REGISTRYINDEX); // pops
}


void LuaBotAI::PushPlayerUD(lua_State* L) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, userDataPlayerRef);
}


void LuaBotAI::UnrefPlayerUD(lua_State* L) {
    if (userDataPlayerRef != LUA_NOREF && userDataPlayerRef != LUA_REFNIL) {
        luaL_unref(L, LUA_REGISTRYINDEX, userDataPlayerRef);
        userDataPlayerRef = LUA_NOREF; // old ref no longer valid
    }
}


// TESTING

void LuaBotAI::Print() {
    printf("LuaBotAI object. Class = %d, userDataRef = %d\n", me->getClass(), userDataRef);
}






