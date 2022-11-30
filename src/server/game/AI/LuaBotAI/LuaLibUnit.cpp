
#include "LuaLibUnit.h"
#include "LuaLibWorldObj.h"
#include "LuaUtils.h"


void LuaBindsAI::BindUnit(lua_State* L) {
	Unit_CreateMetatable(L);
}


Unit** LuaBindsAI::Unit_GetUnitObject(lua_State* L, int idx) {
	return (Unit**) luaL_checkudwithfield(L, idx, "isUnit");//(Unit**) luaL_checkudata(L, idx, LuaBindsAI::UnitMtName);
}


void LuaBindsAI::Unit_CreateUD(Unit* unit, lua_State* L) {
	// create userdata on top of the stack pointing to a pointer of an AI object
	Unit** unitud = static_cast<Unit**>(lua_newuserdatauv(L, sizeof(Unit*), 0));
	*unitud = unit; // swap the AI object being pointed to to the current instance
	luaL_setmetatable(L, LuaBindsAI::UnitMtName);
}


int LuaBindsAI_Unit_CompareEquality(lua_State* L) {
	WorldObject* obj1 = *LuaBindsAI::WObj_GetWObjObject(L);
	WorldObject* obj2 = *LuaBindsAI::WObj_GetWObjObject(L, 2);
	lua_pushboolean(L, obj1 == obj2);
	return 1;
}
void LuaBindsAI::Unit_CreateMetatable(lua_State* L) {
	luaL_newmetatable(L, LuaBindsAI::UnitMtName);
	lua_pushvalue(L, -1); // copy mt cos setfield pops
	lua_setfield(L, -1, "__index"); // mt.__index = mt
	luaL_setfuncs(L, Unit_BindLib, 0); // copy funcs
	lua_pushcfunction(L, LuaBindsAI_Unit_CompareEquality);
	lua_setfield(L, -2, "__eq");
	lua_pushboolean(L, true);
	lua_setfield(L, -2, "isWorldObject");
	lua_pushboolean(L, true);
	lua_setfield(L, -2, "isUnit");
	lua_pop(L, 1); // pop mt
}


// ===================================================
// 1. General
// ===================================================

int LuaBindsAI::Unit_ClearUnitState(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    uint32 state = luaL_checkinteger(L, 2);
    unit->ClearUnitState(state);
    return 0;
}


int LuaBindsAI::Unit_GetClass(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    lua_pushinteger(L, unit->getClass());
    return 1;
}


int LuaBindsAI::Unit_GetLevel(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    lua_pushinteger(L, unit->getLevel());
    return 1;
}


int LuaBindsAI::Unit_GetName(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    lua_pushstring(L, unit->GetName().c_str());
    return 1;
}


int LuaBindsAI::Unit_GetObjectGuid(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    lua_pushstring(L, std::to_string(unit->GetGUID().GetRawValue()).c_str());
    //printf("%s\n", std::to_string(unit->GetObjectGuid().GetRawValue()).c_str());
    return 1;
}


int LuaBindsAI::Unit_GetPowerType(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    lua_pushinteger(L, unit->getPowerType());
    return 1;
}


int LuaBindsAI::Unit_GetRace(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    lua_pushinteger(L, unit->getRace());
    return 1;
}


int LuaBindsAI::Unit_GetTargetGuid(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    lua_pushstring(L, std::to_string(unit->GetTarget().GetRawValue()).c_str());
    return 1;
}


int LuaBindsAI::Unit_HasUnitState(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    uint32 state = luaL_checkinteger(L, 2);
    lua_pushboolean(L, unit->HasUnitState(state));
    return 1;
}


int LuaBindsAI::Unit_IsInDisallowedMountForm(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    lua_pushboolean(L, unit->IsInDisallowedMountForm());
    return 1;
}


int LuaBindsAI::Unit_IsInDungeon(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    lua_pushboolean(L, unit->GetMap()->IsDungeon());
    return 1;
}


int LuaBindsAI::Unit_IsPlayer(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    lua_pushboolean(L, unit->IsPlayer());
    return 1;
}


// ===================================================
// 2. Movement
// ===================================================

int LuaBindsAI::Unit_GetCurrentMovementGeneratorType(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    lua_pushinteger(L, unit->GetMotionMaster()->GetCurrentMovementGeneratorType());
    return 1;
}


int LuaBindsAI::Unit_GetSpeedRate(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    lua_pushnumber(L, unit->GetSpeedRate(MOVE_RUN));
    return 1;
}


int LuaBindsAI::Unit_GetStandState(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    lua_pushinteger(L, unit->getStandState());
    return 1;
}


int LuaBindsAI::Unit_IsMounted(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    lua_pushboolean(L, unit->IsMounted());
    return 1;
}


int LuaBindsAI::Unit_IsMoving(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    lua_pushboolean(L, unit->isMoving());
    return 1;
}


int LuaBindsAI::Unit_IsStopped(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    lua_pushboolean(L, unit->IsStopped());
    return 1;
}


int LuaBindsAI::Unit_MonsterMove(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    float x = luaL_checknumber(L, 2);
    float y = luaL_checknumber(L, 3);
    float z = luaL_checknumber(L, 4);
    float speed = luaL_checknumber(L, 6);
    unit->MonsterMoveWithSpeed(x, y, z, speed);
    return 0;
}


int LuaBindsAI::Unit_MotionMasterClear(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    bool reset = luaL_checkboolean(L, 2);
    unit->GetMotionMaster()->Clear(reset);
    unit->GetMotionMaster()->MoveIdle();
    return 0;
}


int LuaBindsAI::Unit_MoveChase(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    Unit* target = *Unit_GetUnitObject(L, 2);
    float dist = luaL_checknumber(L, 3);
    float angle = luaL_checknumber(L, 4);
    unit->GetMotionMaster()->MoveChase(target, dist, angle);
    return 0;
}


int LuaBindsAI::Unit_MoveFollow(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    Unit* target = *Unit_GetUnitObject(L, 2);
    float dist = luaL_checknumber(L, 3);
    float angle = luaL_checknumber(L, 4);
    unit->GetMotionMaster()->MoveFollow(target, dist, angle);
    return 0;
}


int LuaBindsAI::Unit_MoveIdle(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    unit->GetMotionMaster()->MoveIdle();
    return 0;
}


int LuaBindsAI::Unit_MovePoint(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    uint32 data = luaL_checkinteger(L, 2);
    float x = luaL_checknumber(L, 3);
    float y = luaL_checknumber(L, 4);
    float z = luaL_checknumber(L, 5);
    unit->GetMotionMaster()->MovePoint(data, x, y, z);
    return 0;
}


int LuaBindsAI::Unit_SetStandState(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    uint8 state = luaL_checkinteger(L, 2);
    unit->SetStandState(state);
    return 0;
}


int LuaBindsAI::Unit_StopMoving(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    unit->StopMoving();
    return 0;
}


int LuaBindsAI::Unit_UpdateSpeed(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    float speed = luaL_checknumber(L, 2);
    unit->SetSpeed(MOVE_RUN, speed, false);
    return 0;
}


// ===================================================
// 3. Position
// ===================================================

int LuaBindsAI::Unit_GetDistance(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    WorldObject* to = *WObj_GetWObjObject(L, 2);
    lua_pushnumber(L, unit->GetDistance(to));
    return 1;
}


int LuaBindsAI::Unit_GetDistanceToPos(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    //WorldObject* to = *WObj_GetWObjObject(L, 2);
    float x = luaL_checknumber(L, 2);
    float y = luaL_checknumber(L, 3);
    float z = luaL_checknumber(L, 4);
    lua_pushnumber(L, unit->GetDistance(x, y, z));
    return 1;
}


int LuaBindsAI::Unit_GetGroundHeight(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    float x = luaL_checknumber(L, 2);
    float y = luaL_checknumber(L, 3);
    float z;
    unit->UpdateGroundPositionZ(x, y, z);
    lua_pushnumber(L, z);
    return 1;
}


int LuaBindsAI::Unit_GetMapId(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    lua_pushinteger(L, unit->GetMapId());
    return 1;
}


int LuaBindsAI::Unit_GetNearPointAroundPosition(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    float x = luaL_checknumber(L, 2);
    float y = luaL_checknumber(L, 3);
    float z = luaL_checknumber(L, 4);
    float bounding_radius = luaL_checknumber(L, 5);
    float distance2d = luaL_checknumber(L, 6);
    float absAngle = luaL_checknumber(L, 7);

    float outx = x;
    float outy = y;
    float outz = z;
    Position startPos(x, y, z);

    unit->GetNearPoint(unit, outx, outy, outz, bounding_radius, distance2d, absAngle, 0.0f, &startPos);
    lua_pushnumber(L, outx);
    lua_pushnumber(L, outy);
    lua_pushnumber(L, outz);

    return 3;

}


int LuaBindsAI::Unit_GetPosition(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    //lua_newtable(L);
    lua_pushnumber(L, unit->GetPositionX());
    //lua_setfield(L, -2, "x");
    lua_pushnumber(L, unit->GetPositionY());
    //lua_setfield(L, -2, "y");
    lua_pushnumber(L, unit->GetPositionZ());
    //lua_setfield(L, -2, "z");
    return 3;
}


int LuaBindsAI::Unit_GetZoneId(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    lua_pushinteger(L, unit->GetZoneId());
    return 1;
}


int LuaBindsAI::Unit_IsInWorld(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    lua_pushboolean(L, unit->IsInWorld());
    return 1;
}


int LuaBindsAI::Unit_IsWithinLOSInMap(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    WorldObject* to = *WObj_GetWObjObject(L, 2);
    lua_pushboolean(L, unit->IsWithinLOSInMap(to));
    return 1;
}


int LuaBindsAI::Unit_SetFacingTo(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    float ori = luaL_checknumber(L, 2);
    unit->SetFacingTo(ori);
    return 0;
}


int LuaBindsAI::Unit_SetFacingToObject(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    WorldObject* pObject = *WObj_GetWObjObject(L, 2);
    unit->SetFacingToObject(pObject);
    return 0;
}


// ===================================================
// 4. Death
// ===================================================


int LuaBindsAI::Unit_GetDeathState(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    lua_pushinteger(L, unit->getDeathState());
    return 1;
}


int LuaBindsAI::Unit_IsAlive(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    lua_pushboolean(L, unit->IsAlive());
    return 1;
}


int LuaBindsAI::Unit_IsDead(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    lua_pushboolean(L, unit->isDead());
    return 1;
}


// ===================================================
// 5. Combat Spells
// ===================================================


int LuaBindsAI::Unit_Print(lua_State* L) {
	printf("Unit userdata test\n");
	return 0;
}





