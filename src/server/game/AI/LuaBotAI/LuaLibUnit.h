#ifndef MANGOS_LuaBotUnit_H
#define MANGOS_LuaBotUnit_H

#include "lua.hpp"

namespace LuaBindsAI {

	static const char* UnitMtName = "LuaAI.Unit";

	// Creates metatable for the AI userdata with name specified by AI::Unit_MTNAME
	void BindUnit(lua_State* L);
	// Creates metatable for the AI userdata with name specified by AI::Unit_MTNAME.
	// Registers all the functions listed in LuaBindsBot::Unit_BindLib with that metatable.
	void Unit_CreateMetatable(lua_State* L);
	Unit** Unit_GetUnitObject(lua_State* L, int idx = 1);
	void Unit_CreateUD(Unit* unit, lua_State* L);

    // General
    int Unit_ClearUnitState(lua_State* L);
    int Unit_GetClass(lua_State* L);
    int Unit_GetLevel(lua_State* L);
    int Unit_GetName(lua_State* L);
    int Unit_GetObjectGuid(lua_State* L);
    int Unit_GetPowerType(lua_State* L);
    int Unit_GetRace(lua_State* L);
    int Unit_GetTargetGuid(lua_State* L);
    int Unit_HasUnitState(lua_State* L);
    int Unit_IsInDisallowedMountForm(lua_State* L);
    int Unit_IsInDungeon(lua_State* L);
    int Unit_IsPlayer(lua_State* L);

    // movement related
    int Unit_GetCurrentMovementGeneratorType(lua_State* L);
    int Unit_GetSpeedRate(lua_State* L);
    int Unit_GetStandState(lua_State* L);
    int Unit_IsMounted(lua_State* L);
    int Unit_IsMoving(lua_State* L);
    int Unit_IsStopped(lua_State* L);
    int Unit_MonsterMove(lua_State* L);
    int Unit_MotionMasterClear(lua_State* L);
    int Unit_MoveFollow(lua_State* L);
    int Unit_MoveChase(lua_State* L);
    int Unit_MoveIdle(lua_State* L);
    int Unit_MovePoint(lua_State* L);
    int Unit_SetStandState(lua_State* L);
    int Unit_StopMoving(lua_State* L);
    int Unit_UpdateSpeed(lua_State* L);

    // position related
    int Unit_GetDistance(lua_State* L);
    int Unit_GetDistanceToPos(lua_State* L);
    int Unit_GetGroundHeight(lua_State* L);
    int Unit_GetMapId(lua_State* L);
    int Unit_GetNearPointAroundPosition(lua_State* L);
    int Unit_GetPosition(lua_State* L);
    int Unit_GetZoneId(lua_State* L);
    int Unit_IsInWorld(lua_State* L);
    int Unit_IsWithinLOSInMap(lua_State* L);
    int Unit_SetFacingTo(lua_State* L);
    int Unit_SetFacingToObject(lua_State* L);

    // death related
    int Unit_GetDeathState(lua_State* L);
    int Unit_IsAlive(lua_State* L);
    int Unit_IsDead(lua_State* L);


    int Unit_Print(lua_State* L);


	static const struct luaL_Reg Unit_BindLib[]{
		{"Print", Unit_Print},

        //gen info
        {"ClearUnitState", Unit_ClearUnitState},
        {"GetClass", Unit_GetClass},
        {"GetLevel", Unit_GetLevel},
        {"GetName", Unit_GetName},
        {"GetObjectGuid", Unit_GetObjectGuid},
        {"GetPowerType", Unit_GetPowerType},
        {"GetRace", Unit_GetRace},
        {"GetTargetGuid", Unit_GetTargetGuid},
        {"HasUnitState", Unit_HasUnitState},
        {"IsInDisallowedMountForm", Unit_IsInDisallowedMountForm},
        {"IsInDungeon", Unit_IsInDungeon},
        {"IsPlayer", Unit_IsPlayer},

        // movement related
        {"GetCurrentMovementGeneratorType", Unit_GetCurrentMovementGeneratorType},
        {"GetStandState", Unit_GetStandState},
        {"GetSpeedRate", Unit_GetSpeedRate},
        {"IsMounted", Unit_IsMounted},
        {"IsMoving", Unit_IsMoving},
        {"IsStopped", Unit_IsStopped},
        {"MonsterMove", Unit_MonsterMove},
        {"MotionMasterClear", Unit_MotionMasterClear},
        {"MoveChase", Unit_MoveChase},
        {"MoveFollow", Unit_MoveFollow},
        {"MoveIdle", Unit_MoveIdle},
        {"MovePoint", Unit_MovePoint},
        {"SetStandState", Unit_SetStandState},
        {"StopMoving", Unit_StopMoving},
        {"UpdateSpeed", Unit_UpdateSpeed},

        // position related
        {"GetDistance", Unit_GetDistance},
        {"GetDistanceToPos", Unit_GetDistanceToPos},
        {"GetGroundHeight", Unit_GetGroundHeight},
        {"GetMapId", Unit_GetMapId},
        {"GetNearPointAroundPosition", Unit_GetNearPointAroundPosition},
        {"GetPosition", Unit_GetPosition},
        {"GetZoneId", Unit_GetZoneId},
        {"IsInWorld", Unit_IsInWorld},
        {"IsWithinLOSInMap", Unit_IsWithinLOSInMap},
        {"SetFacingTo", Unit_SetFacingTo},
        {"SetFacingToObject", Unit_SetFacingToObject},

        // death related
        {"GetDeathState", Unit_GetDeathState},
        {"IsAlive", Unit_IsAlive},
        {"IsDead", Unit_IsDead},


        {NULL, NULL}
	};

}


#endif
