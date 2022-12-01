
#include "LuaLibUnit.h"
#include "LuaLibWorldObj.h"
#include "LuaUtils.h"
#include "SpellMgr.h"
#include "Auras/SpellAuras.h"
#include "SpellAuraEffects.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"


namespace MaNGOS {
    class AnyHostileUnit {
        WorldObject const* i_obj;
        const Unit* hostileTo;
        float i_range;
        bool b_3dDist;
    public:
        AnyHostileUnit(WorldObject const* obj, const Unit* hostileTo, float range, bool distance_3d = true) : hostileTo(hostileTo), i_obj(obj), i_range(range), b_3dDist(distance_3d) {}
        WorldObject const& GetFocusObject() const { return *i_obj; }
        bool operator()(Unit* u) {
            return u->IsAlive() && i_obj->IsWithinDistInMap(u, i_range, b_3dDist) && u->IsHostileTo(hostileTo);
        }
    };
}




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
// 5. Combat
// ===================================================


// -----------------------------------------------------------
//                      Combat RELATED
// -----------------------------------------------------------

int LuaBindsAI::Unit_AddAura(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    int spellId = luaL_checkinteger(L, 2);
    unit->AddAura(spellId, unit);
    return 0;
}


int LuaBindsAI::Unit_CastSpell(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    Unit* target = *Unit_GetUnitObject(L, 2);
    int spellId = luaL_checkinteger(L, 3);
    bool triggered = luaL_checkboolean(L, 4);
    lua_pushinteger(L, unit->CastSpell(target, spellId, triggered));
    return 1;
}


int LuaBindsAI::Unit_ClearTarget(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    unit->SetTarget();
    return 0;
}


int LuaBindsAI::Unit_GetAttackersTbl(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    lua_newtable(L);
    int tblIdx = 1;
    for (const auto pAttacker : unit->getAttackers())
        if (IsValidHostileTarget(unit, pAttacker)) {
            Unit_CreateUD(pAttacker, L); // pushes pAttacker userdata on top of stack
            lua_seti(L, -2, tblIdx); // stack[-2][tblIdx] = stack[-1], pops pAttacker
            tblIdx++;
        }
    return 1;
}


int LuaBindsAI::Unit_GetAttackRange(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    Unit* toAttack = *Unit_GetUnitObject(L, 2);
    if (Creature* c = unit->ToCreature())
        lua_pushnumber(L, c->GetAttackDistance(toAttack));
    else
        lua_pushnumber(L, 0);
    return 1;
}


int LuaBindsAI::Unit_GetAttackTimer(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    int type = luaL_checkinteger(L, 2);
    if (type < WeaponAttackType::BASE_ATTACK || type > WeaponAttackType::RANGED_ATTACK)
        luaL_error(L, "Unit.GetAttackTimer invalid attack type %d, allowed values [%d, %d]", type, BASE_ATTACK, RANGED_ATTACK);
    lua_pushinteger(L, unit->getAttackTimer((WeaponAttackType) type));
    return 1;
}


int LuaBindsAI::Unit_GetAuraStacks(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    uint32 spellId = luaL_checkinteger(L, 2);
    if (Aura* sah = unit->GetAura(spellId))
        lua_pushinteger(L, sah->GetStackAmount());
    else
        lua_pushinteger(L, -1);
    return 1;
}


int LuaBindsAI::Unit_GetCombatDistance(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    Unit* to = *Unit_GetUnitObject(L, 2);

    float dx = unit->GetPositionX() - to->GetPositionX();
    float dy = unit->GetPositionY() - to->GetPositionY();
    float dz = unit->GetPositionZ() - to->GetPositionZ();
    float distsq = dx * dx + dy * dy + dz * dz;

    float sizefactor = unit->GetCombatReach() + to->GetCombatReach();
    float maxdist = distsq + sizefactor;

    lua_pushinteger(L, maxdist);
    return 1;
}


int LuaBindsAI::Unit_GetEnemyCountInRadiusAround(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    Unit* to = *Unit_GetUnitObject(L, 2);
    float r = luaL_checknumber(L, 3);
    lua_pushinteger(L, unit->GetEnemyCountInRadiusAround(to, r));
    return 1;
}


int LuaBindsAI::Unit_GetHealth(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    lua_pushinteger(L, unit->GetHealth());
    return 1;
}


int LuaBindsAI::Unit_GetMaxHealth(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    lua_pushinteger(L, unit->GetMaxHealth());
    return 1;
}


int LuaBindsAI::Unit_GetHealthPercent(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    lua_pushnumber(L, unit->GetHealthPct());
    return 1;
}


int LuaBindsAI::Unit_GetMeleeReach(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    lua_pushnumber(L, unit->GetMeleeReach());
    return 1;
}




int LuaBindsAI::Unit_GetPower(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    int powerId = luaL_checkinteger(L, 2);
    if (powerId < POWER_MANA || powerId > POWER_HAPPINESS)
        luaL_error(L, "Unit.GetPower. Invalid power id, expected value in range [0, 4], got %d", powerId);
    lua_pushinteger(L, unit->GetPower((Powers) powerId));
    return 1;
}


int LuaBindsAI::Unit_GetMaxPower(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    int powerId = luaL_checkinteger(L, 2);
    if (powerId < POWER_MANA || powerId > POWER_HAPPINESS)
        luaL_error(L, "Unit.GetMaxPower. Invalid power id, expected value in range [0, 4], got %d", powerId);
    lua_pushinteger(L, unit->GetMaxPower((Powers) powerId));
    return 1;
}


int LuaBindsAI::Unit_GetPowerPercent(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    int powerId = luaL_checkinteger(L, 2);
    if (powerId < POWER_MANA || powerId > POWER_HAPPINESS)
        luaL_error(L, "Unit.GetPowerPercent. Invalid power id, expected value in range [0, 4], got %d", powerId);
    lua_pushnumber(L, unit->GetPowerPct((Powers) powerId));
    return 1;
}


int LuaBindsAI::Unit_GetSpellCost(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    uint32 spellId = luaL_checkinteger(L, 2);
    const SpellInfo* spell = sSpellMgr->GetSpellInfo(spellId);
    if (spell)
        lua_pushinteger(L, spell->CalcPowerCost(unit, spell->GetSchoolMask()));
    else {
        luaL_error(L, "Unit.GetSpellCost: spell %d doesn't exist", spellId);
        lua_pushnil(L); // technically never reached
    }
    return 1;
}


int LuaBindsAI::Unit_GetShapeshiftForm(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    lua_pushinteger(L, unit->GetShapeshiftForm());
    return 1;
}


int LuaBindsAI::Unit_GetThreat(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    Unit* victim = *Unit_GetUnitObject(L, 2);
    lua_pushnumber(L, unit->GetThreatMgr().GetThreat(victim));
    return 1;
}


int LuaBindsAI::Unit_GetThreatTbl(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    lua_newtable(L);
    int tblIdx = 1;
    for (auto v : unit->GetThreatMgr().GetThreatList()) {
        if (Unit* hostile = v->getTarget()) {
            Unit_CreateUD(hostile, L);
            lua_seti(L, -2, tblIdx); // stack[1][tblIdx] = stack[-1], pops unit
            tblIdx++;
        }
    }
    return 1;
}


int LuaBindsAI::Unit_GetVictim(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    lua_pushunitornil(L, unit->GetVictim());
    return 1;
}


int LuaBindsAI::Unit_GetVictimsInRange(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    Unit* hostileTo = *Unit_GetUnitObject(L, 2);
    float range = luaL_checknumber(L, 3);

    lua_newtable(L);

    std::list<Unit*> lList;
    MaNGOS::AnyHostileUnit check(unit, hostileTo, range);
    Acore::UnitListSearcher<MaNGOS::AnyHostileUnit> searcher(unit, lList, check);
    Cell::VisitAllObjects(unit, searcher, range);

    int i = 1;
    for (auto unit : lList) {
        Unit_CreateUD(unit, L);
        lua_seti(L, -2, i);
        i++;
    }
    return 1;
}


int LuaBindsAI::Unit_GetCurrentSpellId(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    int spellType = luaL_checkinteger(L, 2);
    if (spellType < CURRENT_MELEE_SPELL || spellType > CURRENT_CHANNELED_SPELL)
        luaL_error(L, "Unit.GetCurrentSpellId. Invalid spell type id, expected value in range [0, 3], got %d", spellType);
    if (Spell* curSpell = unit->GetCurrentSpell((CurrentSpellTypes) spellType))
        lua_pushinteger(L, curSpell->m_spellInfo->Id);
    else
        lua_pushinteger(L, -1);
    return 1;
}


int LuaBindsAI::Unit_HasAura(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    int spellId = luaL_checkinteger(L, 2);
    lua_pushboolean(L, unit->HasAura(spellId));
    return 1;
}


int LuaBindsAI::Unit_HasAuraType(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    int auraId = luaL_checkinteger(L, 2);
    if (auraId < SPELL_AURA_NONE || auraId >= TOTAL_AURAS)
        luaL_error(L, "Unit.RemoveSpellsCausingAura. Invalid aura type id, expected value in range [%d, %d], got %d", SPELL_AURA_NONE, TOTAL_AURAS - 1, auraId);
    lua_pushboolean(L, unit->HasAuraType((AuraType) auraId));
    return 1;
}


int LuaBindsAI::Unit_HasAuraBy(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    Unit* by = *Unit_GetUnitObject(L, 2);
    int auraId = luaL_checkinteger(L, 3);
    if (auraId < SPELL_AURA_NONE || auraId >= TOTAL_AURAS)
        luaL_error(L, "Unit.RemoveSpellsCausingAura. Invalid aura type id, expected value in range [%d, %d], got %d", SPELL_AURA_NONE, TOTAL_AURAS - 1, auraId);
    uint32 spellId = luaL_checkinteger(L, 4);
    lua_pushboolean(L, unit->HasAuraByCaster((AuraType) auraId, spellId, by->GetGUID()));
    return 1;
}


int LuaBindsAI::Unit_HasObjInArc(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    WorldObject* obj = *WObj_GetWObjObject(L, 2);
    float arc = luaL_checknumber(L, 3);
    Position pos(obj->GetPosition());
    lua_pushboolean(L, unit->HasInArc(arc, &pos, 0.0f));
    return 1;
}


int LuaBindsAI::Unit_HasPosInArc(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    float arc = luaL_checknumber(L, 2);
    float x = luaL_checknumber(L, 3);
    float y = luaL_checknumber(L, 4);
    float z;
    unit->UpdateGroundPositionZ(x, y, z);
    Position pos(x, y, z);
    lua_pushboolean(L, unit->HasInArc(arc, &pos));
    return 1;
}


int LuaBindsAI::Unit_InterruptSpell(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    int spellType = luaL_checkinteger(L, 2);
    bool withDelayed = luaL_checkboolean(L, 3);
    if (spellType < CURRENT_MELEE_SPELL || spellType > CURRENT_CHANNELED_SPELL)
        luaL_error(L, "Unit.InterruptSpell. Invalid spell type id, expected value in range [0, 3], got %d", spellType);
    unit->InterruptSpell((CurrentSpellTypes) spellType, withDelayed);
    return 0;
}


int LuaBindsAI::Unit_IsNonMeleeSpellCasted(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    bool withDelayed = luaL_checkboolean(L, 2);
    bool skipChanneled = luaL_checkboolean(L, 3);
    bool skipAutorepeat = luaL_checkboolean(L, 4);
    lua_pushboolean(L, unit->IsNonMeleeSpellCast(withDelayed, skipChanneled, skipAutorepeat, false, false));
    return 1;
}


int LuaBindsAI::Unit_IsInCombat(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    lua_pushboolean(L, unit->IsInCombat());
    return 1;
}


int LuaBindsAI::Unit_IsTargetInRangeOfSpell(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    Unit* pTarget = *Unit_GetUnitObject(L, 2);
    uint32 spellId = luaL_checkinteger(L, 3);

    if (const SpellInfo* spell = sSpellMgr->GetSpellInfo(spellId))
        if (unit != pTarget && spell->Effects[0].GetImplicitTargetType() != EFFECT_IMPLICIT_TARGET_CASTER) {

            float maxR = spell->GetMaxRange(spell->IsPositive(), unit);
            float minR = spell->GetMinRange(spell->IsPositive());

            if (!unit->IsWithinCombatRange(pTarget, maxR) || unit->IsWithinCombatRange(pTarget, minR)) {
                lua_pushboolean(L, false);
                return 1;
            }
        }
    else
        luaL_error(L, "Unit.IsTargetInRangeOfSpell spell doesn't exist. Id = %d", spellId);

    lua_pushboolean(L, true);
    return 1;
}


int LuaBindsAI::Unit_IsValidHostileTarget(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    Unit* target = *Unit_GetUnitObject(L, 2);
    lua_pushboolean(L, IsValidHostileTarget(unit, target));
    return 1;
}


int LuaBindsAI::Unit_RemoveAura(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    uint32 auraId = luaL_checkinteger(L, 2);
    if (Aura* aura = unit->GetAura(auraId))
        unit->RemoveAura(aura);
    return 0;
}


int LuaBindsAI::Unit_RemoveSpellsCausingAura(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    int auraId = luaL_checkinteger(L, 2);
    if (auraId < SPELL_AURA_NONE || auraId >= TOTAL_AURAS)
        luaL_error(L, "Unit.RemoveSpellsCausingAura. Invalid aura type id, expected value in range [%d, %d], got %d", SPELL_AURA_NONE, TOTAL_AURAS - 1, auraId);
    unit->RemoveAurasByType((AuraType) auraId);
    return 0;
}


int LuaBindsAI::Unit_SetHealth(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    uint32 health = luaL_checkinteger(L, 2);
    unit->SetHealth(health);
    return 0;
}


int LuaBindsAI::Unit_SetMaxHealth(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    uint32 health = luaL_checkinteger(L, 2);
    unit->SetMaxHealth(health);
    return 0;
}


int LuaBindsAI::Unit_SetHealthPercent(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    float health = luaL_checknumber(L, 2) / 100.0f;
    uint32 maxH = unit->GetMaxHealth();
    unit->SetHealth(health * maxH);
    return 0;
}


int LuaBindsAI::Unit_SetMaxPower(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    uint32 power = luaL_checkinteger(L, 2);
    int powerId = luaL_checkinteger(L, 3);
    if (powerId < POWER_MANA || powerId > POWER_RUNIC_POWER)
        luaL_error(L, "Unit.SetMaxPower. Invalid power id, expected value in range [0, 4], got %d", powerId);
    unit->SetMaxPower((Powers) powerId, power);
    return 0;
}


int LuaBindsAI::Unit_SetPower(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    uint32 power = luaL_checkinteger(L, 2);
    int powerId = luaL_checkinteger(L, 3);
    if (powerId < POWER_MANA || powerId > POWER_RUNIC_POWER)
        luaL_error(L, "Unit.SetPower. Invalid power id, expected value in range [0, 4], got %d", powerId);
    unit->SetPower((Powers) powerId, power);
    return 0;
}


int LuaBindsAI::Unit_SetPowerPercent(lua_State* L) {
    Unit* unit = *Unit_GetUnitObject(L);
    float power = luaL_checknumber(L, 2) / 100.0f;
    int powerId = luaL_checkinteger(L, 3);
    if (powerId < POWER_MANA || powerId > POWER_RUNIC_POWER)
        luaL_error(L, "Unit.SetPowerPercent. Invalid power id, expected value in range [0, 4], got %d", powerId);
    uint32 maxP = unit->GetMaxPower(Powers(powerId));
    unit->SetPower((Powers) powerId, power * maxP);
    return 0;
}



int LuaBindsAI::Unit_Print(lua_State* L) {
	printf("Unit userdata test\n");
	return 0;
}

// ==================================================================

bool Unit::HasAuraByCaster(AuraType auraType, uint32 spellId, ObjectGuid casterGuid) const {
    for (const auto& iter : m_modAuras[auraType])
        if (iter->GetCasterGUID() == casterGuid && iter->GetId() == spellId)
            return true;
    return false;

}






