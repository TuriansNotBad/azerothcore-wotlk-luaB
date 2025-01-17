
#include "LuaBotManager.h"
#include "LuaUtils.h"
#include "LuaLibAI.h"
#include "LuaBotAI.h"
#include "LuaLibUnit.h"
#include "Player.h"
#include "TargetedMovementGenerator.h"
#include "MovementGenerator.h"
#include "Pet.h"
#include "CreatureAI.h"
#include "Spell.h"


void LuaBindsAI::BindAI( lua_State* L ) {
	AI_CreateMetatable( L );
}


LuaBotAI** LuaBindsAI::AI_GetAIObject( lua_State* L ) {
	return (LuaBotAI**) luaL_checkudata( L, 1, LuaBotAI::MTNAME );
}


void LuaBindsAI::AI_CreateMetatable( lua_State* L ) {
	luaL_newmetatable( L, LuaBotAI::MTNAME );
	lua_pushvalue( L, -1 ); // copy mt cos setfield pops
	lua_setfield( L, -1, "__index" ); // mt.__index = mt
	luaL_setfuncs( L, AI_BindLib, 0 ); // copy funcs
	lua_pop( L, 1 ); // pop mt
}


int LuaBindsAI::AI_AddTopGoal(lua_State* L) {

	int nArgs = lua_gettop(L);

	if (nArgs < 3) {
		luaL_error(L, "AI.AddTopGoal - invalid number of arguments. 3 min, %d given", nArgs);
		//return 0;
	}

	LuaBotAI** ai = AI_GetAIObject(L);
	int goalId = luaL_checknumber(L, 2);

	Goal* topGoal = (*ai)->GetTopGoal();

	if (goalId != topGoal->GetGoalId() || topGoal->GetTerminated()) {
		double life = luaL_checknumber(L, 3);

		// (*goal)->Print();

		std::vector<GoalParamP> params;
		Goal_GrabParams(L, nArgs, params);
		// printf( "%d\n", params.size() );

		Goal** goalUserdata = Goal_CreateGoalUD(L, (*ai)->AddTopGoal(goalId, life, params, L)); // ud on top of the stack
		// duplicate userdata for return result
		lua_pushvalue(L, -1);
		// save userdata
		(*goalUserdata)->SetRef(luaL_ref(L, LUA_REGISTRYINDEX)); // pops the object as well
		(*goalUserdata)->CreateUsertable();
	}
	else {
		// leave topgoal's userdata as return result for lua
		lua_rawgeti(L, LUA_REGISTRYINDEX, topGoal->GetRef());
	}

	return 1;

}


int LuaBindsAI::AI_HasTopGoal(lua_State* L) {
	LuaBotAI* ai = *AI_GetAIObject(L);
	int id = luaL_checkinteger(L, 2);
	if (ai->GetTopGoal()->GetTerminated())
		lua_pushboolean(L, false);
	else
		lua_pushboolean(L, ai->GetTopGoal()->GetGoalId() == id);
	return 1;
}



int LuaBindsAI::AI_GetPlayer(lua_State* L) {
	LuaBotAI* ai = *AI_GetAIObject(L);
	ai->PushPlayerUD(L);
	return 1;
}


int LuaBindsAI::AI_Print( lua_State* L ) {
	LuaBotAI** ai = AI_GetAIObject( L );
	( *ai )->Print();
	return 0;
}


int LuaBindsAI::AI_GetUserTbl(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    lua_rawgeti(L, LUA_REGISTRYINDEX, ai->GetUserTblRef());
    return 1;
}


int LuaBindsAI::AI_DrinkAndEat(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    float healthPer = luaL_checknumber(L, 2);
    float manaPer = luaL_checknumber(L, 3);
    lua_pushboolean(L, ai->DrinkAndEat(healthPer, manaPer));
    return 1;
}


int LuaBindsAI::AI_IsReady(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    lua_pushboolean(L, ai->IsReady());
    return 1;
}


int LuaBindsAI::AI_IsInitialized(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    lua_pushboolean(L, ai->IsInitalized());
    return 1;
}

// -----------------------------------------------------------
//                      BGS
// -----------------------------------------------------------

int LuaBindsAI::AI_HasBgInvite(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    lua_pushboolean(L, ai->HasBgInvite());
    return 1;
}


int LuaBindsAI::AI_AcceptBgInvite(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    ai->AcceptBgInvite();
    return 0;
}


int LuaBindsAI::AI_LeaveBattlefield(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);

    if (!ai->me->InBattleground())
        return 0;

    WorldPacket data(CMSG_LEAVE_BATTLEFIELD);
    data << uint8(0);                           // unk1
    data << uint8(0);                           // unk2
    data << uint32(0);                           // BattleGroundTypeId
    data << uint16(0);                          // unk3
    ai->me->GetSession()->HandleBattlefieldLeaveOpcode(data);

    return 0;
}


int LuaBindsAI::AI_ShouldLeaveBg(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    lua_pushboolean(L, ai->ShouldLeaveBg());
    return 1;
}


int LuaBindsAI::AI_IsBgObjSpawned(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    int type = luaL_checkinteger(L, 2);

    if (!ai->me->InBattleground())
        return 0;

    if (Battleground* battleground = ai->me->GetBattleground(false)) {

        if (type < 0 || type >= battleground->BgObjects.size())
            luaL_error(L, "AI_IsBgObjSpawned: index out of bounds. [0, %d), got %d", battleground->BgObjects.size(), type);

        if (GameObject* go = battleground->GetBGObject(type)) {
            lua_pushboolean(L, go->isSpawned());
            return 1;
        }

    }

    return 0;
}


int LuaBindsAI::AI_GetBgObjPos(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    int type = luaL_checkinteger(L, 2);

    if (!ai->me->InBattleground())
        return 0;

    if (Battleground* battleground = ai->me->GetBattleground(false)) {

        if (type < 0 || type >= battleground->BgObjects.size())
            luaL_error(L, "AI_GetBgObjPos: index out of bounds. [0, %d), got %d", battleground->BgObjects.size(), type);

        if (GameObject* go = battleground->GetBGObject(type)) {
            if (go->isSpawned()) {
                lua_pushnumber(L, go->GetPositionX());
                lua_pushnumber(L, go->GetPositionY());
                lua_pushnumber(L, go->GetPositionZ());
                return 3;
            }
        }

    }

    return 0;
}


int LuaBindsAI::AI_UseNearBgObj(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    int type = luaL_checkinteger(L, 2);

    if (!ai->me->InBattleground())
        return 0;

    if (Battleground* battleground = ai->me->GetBattleground(false)) {

        if (type < 0 || type >= battleground->BgObjects.size())
            luaL_error(L, "AI_UseNearBgObj: index out of bounds. [0, %d), got %d", battleground->BgObjects.size(), type);

        if (GameObject* go = battleground->GetBGObject(type)) {
            if (go->isSpawned() && ai->me->IsWithinDistInMap(go, go->GetInteractionDistance())) {
                WorldPacket data(CMSG_GAMEOBJ_USE);
                data << go->GetGUID();
                ai->me->GetSession()->HandleGameObjectUseOpcode(data);
            }
        }

    }

    return 0;
}


int LuaBindsAI::AI_GetBgCreature(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    int type = luaL_checkinteger(L, 2);

    if (!ai->me->InBattleground())
        return 0;

    if (Battleground* battleground = ai->me->GetBattleground(false)) {

        if (type < 0 || type >= battleground->BgCreatures.size())
            luaL_error(L, "AI_GetBgCreature: index out of bounds. [0, %d), got %d", battleground->BgCreatures.size(), type);

        if (Creature* creature = battleground->GetBGCreature(type)) {
            lua_pushunitornil(L, creature->ToUnit());
            return 1;
        }

    }

    return 0;
}

// -----------------------------------------------------------
//                      Bot Mgmt
// -----------------------------------------------------------

int LuaBindsAI::AI_AddBot(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);

    if (!ai->master)
        return 0;

    std::string char_name = luaL_checkstring(L, 2);
    int logicID = luaL_checkinteger(L, 3);
    std::string spec = luaL_checkstring(L, 4);

    uint32 accountID = sCharacterCache->GetCharacterAccountIdByGuid(ai->master->GetGUID());
    if (!accountID)
        return 0;

    sLuaBotMgr.AddBot(char_name, accountID, logicID, spec);
    return 0;
}


int LuaBindsAI::AI_RemoveBot(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    Unit* toRemove = *Unit_GetUnitObject(L, 2);

    if (!ai->master)
        return 0;

    if (!toRemove->IsPlayer())
        return 0;

    Player* plrToRemove = toRemove->ToPlayer();
    if (!plrToRemove || !plrToRemove->IsLuaBot())
        return 0;

    if (LuaBotAI* aiToRemove = plrToRemove->GetLuaAI())
        aiToRemove->RequestLogOut();

    return 0;
}


int LuaBindsAI::AI_IsBotConnected(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    std::string name = luaL_checkstring(L, 2);
    Player* p = ObjectAccessor::FindPlayerByName(name, false);
    
    if (p)
        lua_pushboolean(L, true);
    else
        lua_pushboolean(L, false);
    return 1;
}


int LuaBindsAI::AI_CanMasterOwnBot(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);

    if (!ai->master) {
        lua_pushboolean(L, false);
        return 1;
    }

    std::string name = luaL_checkstring(L, 2);
    Player* p = ObjectAccessor::FindPlayerByName(name, true);
    if (p) {

        if (p->IsLuaBot()) {
            if (LuaBotAI* botAI = p->GetLuaAI()) {
                lua_pushboolean(L, ai->master->GetGUID() == botAI->master->GetGUID());
                return 1;
            }
        }
        // not a bot or failed to get AI
        lua_pushboolean(L, false);
        return 1;

    }
    lua_pushboolean(L, true);
    return 1;
}

// -----------------------------------------------------------
//                      Combat RELATED
// -----------------------------------------------------------


int LuaBindsAI::AI_AddAmmo(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    ai->AddAmmo();
    return 0;
}


int LuaBindsAI::AI_AttackAutoshot(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    Unit* pVictim = *LuaBindsAI::Unit_GetUnitObject(L, 2);
    float chaseDist = luaL_checknumber(L, 3);
    ai->AttackAutoshot(pVictim, chaseDist);
    return 0;
}


int LuaBindsAI::AI_AttackSet(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    Unit* pVictim = *LuaBindsAI::Unit_GetUnitObject(L, 2);
    bool meleeAttack = luaL_checkboolean(L, 3);
    lua_pushboolean(L, ai->me->Attack(pVictim, meleeAttack));
    return 1;
}


int LuaBindsAI::AI_AttackSetChase(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    Unit* pVictim = *LuaBindsAI::Unit_GetUnitObject(L, 2);
    bool meleeAttack = luaL_checkboolean(L, 3);
    float chaseDist = luaL_checknumber(L, 4);
    bool attack = ai->me->Attack(pVictim, meleeAttack);
    lua_pushboolean(L, attack);
    if (attack)
        ai->me->GetMotionMaster()->MoveChase(pVictim, chaseDist);
    return 1;
}


int LuaBindsAI::AI_AttackStopAutoshot(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    ai->AttackStopAutoshot();
    return 0;
}


int LuaBindsAI::AI_AttackStart(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    Unit* pVictim = *LuaBindsAI::Unit_GetUnitObject(L, 2);
    Player* me = ai->me;

    if (me->IsMounted())
        me->RemoveAurasByType(SPELL_AURA_MOUNTED);

    if (me->Attack(pVictim, true))
    {
        float chaseDist = 1.0f;
        if (ai->GetRole() == ROLE_RDPS &&
            me->GetPowerPct(POWER_MANA) > 10.0f &&
            me->IsWithinCombatRange(pVictim, 8.0f))
            chaseDist = 25.0f;

        me->GetMotionMaster()->MoveChase(pVictim, chaseDist, ai->GetRole() == ROLE_MDPS ? 3.0f : 0.0f);
        lua_pushboolean(L, true);
        return 1;
    }
    lua_pushboolean(L, false);
    return 1;

}


int LuaBindsAI::AI_GetChaseDist(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    if (ai->me->GetMotionMaster()->GetCurrentMovementGeneratorType() == CHASE_MOTION_TYPE) {
        ChaseMovementGenerator<Player>* cmg = dynamic_cast<ChaseMovementGenerator<Player>*>(ai->me->GetMotionMaster()->top());
        if (cmg) {
            if (cmg->GetRange()->has_value()) {
                const ChaseRange cr = cmg->GetRange()->value();
                lua_pushnumber(L, cr.MinRange);
                lua_pushnumber(L, cr.MinTolerance);
                lua_pushnumber(L, cr.MaxRange);
                lua_pushnumber(L, cr.MaxTolerance);
                return 4;
            }
        }
    }
    lua_pushnil(L);
    return 1;
}


int LuaBindsAI::AI_GetChaseAngle(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    if (ai->me->GetMotionMaster()->GetCurrentMovementGeneratorType() == CHASE_MOTION_TYPE) {
        ChaseMovementGenerator<Player>* cmg = dynamic_cast<ChaseMovementGenerator<Player>*>(ai->me->GetMotionMaster()->top());
        if (cmg) {
            if (cmg->GetAngle()->has_value()) {
                const ChaseAngle cr = cmg->GetAngle()->value();
                lua_pushnumber(L, cr.RelativeAngle);
                lua_pushnumber(L, cr.Tolerance);
                return 2;
            }
        }
    }
    lua_pushnil(L);
    return 1;
}


int LuaBindsAI::AI_GetChaseTarget(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    if (ai->me->GetMotionMaster()->GetCurrentMovementGeneratorType() == CHASE_MOTION_TYPE)
        if (ChaseMovementGenerator<Player>* cmg = dynamic_cast<ChaseMovementGenerator<Player>*>(ai->me->GetMotionMaster()->top())) {
            lua_pushunitornil(L, cmg->GetTarget());
            return 1;
        }
    lua_pushnil(L);
    return 1;
}


int LuaBindsAI::AI_UpdateChaseDist(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    float minR = luaL_checknumber(L, 2);
    float minT = luaL_checknumber(L, 3);
    float maxR = luaL_checknumber(L, 4);
    float maxT = luaL_checknumber(L, 5);
    if (ai->me->GetMotionMaster()->GetCurrentMovementGeneratorType() == CHASE_MOTION_TYPE) {
        ChaseMovementGenerator<Player>* cmg = dynamic_cast<ChaseMovementGenerator<Player>*>(ai->me->GetMotionMaster()->top());
        if (cmg)
            cmg->SetRange(minR, minT, maxR, maxT);
    }
    return 0;
}


int LuaBindsAI::AI_UpdateChaseAngle(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    float A = luaL_checknumber(L, 2);
    float T = luaL_checknumber(L, 3);
    if (ai->me->GetMotionMaster()->GetCurrentMovementGeneratorType() == CHASE_MOTION_TYPE) {
        ChaseMovementGenerator<Player>* cmg = dynamic_cast<ChaseMovementGenerator<Player>*>(ai->me->GetMotionMaster()->top());
        if (cmg)
            cmg->SetAngle(A, T);
    }
    return 0;
}


int LuaBindsAI::AI_AttackStop(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    lua_pushboolean(L, ai->me->AttackStop());
    return 1;
}


int LuaBindsAI::AI_IsValidHostileTarget(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    Unit* pTarget = *LuaBindsAI::Unit_GetUnitObject(L, 2);
    lua_pushboolean(L, IsValidHostileTarget(ai->me, pTarget));
    return 1;
}


int LuaBindsAI::AI_CanTryToCastSpell(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    Unit* pTarget = *LuaBindsAI::Unit_GetUnitObject(L, 2);
    int spellId = luaL_checkinteger(L, 3);
    bool bAura = luaL_checkboolean(L, 4);
    if (const SpellInfo* spell = sSpellMgr->GetSpellInfo(spellId))
        lua_pushboolean(L, ai->CanTryToCastSpell(pTarget, spell, bAura));
    else
        luaL_error(L, "AI.CanTryToCastSpell spell doesn't exist. Id = %d", spellId);
    return 1;
}


int LuaBindsAI::AI_DoCastSpell(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    Unit* pTarget = *LuaBindsAI::Unit_GetUnitObject(L, 2);
    int spellId = luaL_checkinteger(L, 3);
    if (const SpellInfo* spell = sSpellMgr->GetSpellInfo(spellId))
        lua_pushinteger(L, ai->DoCastSpell(pTarget, spell));
    else
        luaL_error(L, "AI.DoCastSpell spell doesn't exist. Id = %d", spellId);
    return 1;
}


int LuaBindsAI::AI_IsCastingHeal(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    Spell* pCurrentSpell = ai->me->GetCurrentSpell(CurrentSpellTypes::CURRENT_GENERIC_SPELL);
    if (!pCurrentSpell)
        pCurrentSpell = ai->me->GetCurrentSpell(CurrentSpellTypes::CURRENT_CHANNELED_SPELL);
    if (!pCurrentSpell) {
        lua_pushboolean(L, false);
        return 1;
    }
    
    if (auto pSpellInfo = pCurrentSpell->m_spellInfo)
        lua_pushboolean(L, pSpellInfo->IsPositive());
    else
        lua_pushboolean(L, false);
    return 1;
}


int LuaBindsAI::AI_GetAttackersInRangeCount(lua_State* L) {
    LuaBotAI** ai = AI_GetAIObject(L);
    float r = luaL_checknumber(L, 2);
    lua_pushinteger(L, (*ai)->GetAttackersInRangeCount(r));
    return 1;
}


int LuaBindsAI::AI_GetClass(lua_State* L) {
    LuaBotAI** ai = AI_GetAIObject(L);
    lua_pushinteger(L, (*ai)->me->getClass());
    return 1;
}


int LuaBindsAI::AI_GetMarkedTarget(lua_State* L) {
    LuaBotAI** ai = AI_GetAIObject(L);
    int markId = luaL_checkinteger(L, 2);
    if (markId < RaidTargetIcon::RAID_TARGET_ICON_STAR || markId > RaidTargetIcon::RAID_TARGET_ICON_SKULL)
        luaL_error(L, "AI.GetMarkedTarget - invalid mark id. Acceptable values - [0, 7]. Got %d", markId);
    lua_pushunitornil(L, (*ai)->GetMarkedTarget((RaidTargetIcon) markId));
    return 1;
}


int LuaBindsAI::AI_GetSpec(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    lua_pushstring(L, ai->spec.c_str());
    return 1;
}


int LuaBindsAI::AI_SetSpec(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    std::string spec = luaL_checkstring(L, 2);
    ai->spec = spec;
    return 0;
}


int LuaBindsAI::AI_GetRole(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    lua_pushinteger(L, ai->GetRole());
    return 1;
}


int LuaBindsAI::AI_SetRole(lua_State* L) {
    LuaBotAI** ai = AI_GetAIObject(L);
    int roleID = luaL_checkinteger(L, 2);
    if (roleID <= ROLE_INVALID || roleID > ROLE_HEALER)
        luaL_error(L, "AI.SetRole - invalid role ID. Accepted values [1, %d]. Got %d", ROLE_HEALER, roleID);
    (*ai)->SetRole(roleID);
    return 0;
}


int LuaBindsAI::AI_GetGameTime(lua_State* L) {
    lua_pushinteger(L, getMSTime());
    return 1;
}


int LuaBindsAI::AI_RunAwayFromTarget(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    Unit* pTarget = *LuaBindsAI::Unit_GetUnitObject(L, 2);
    lua_pushboolean(L, ai->RunAwayFromTarget(pTarget));
    return 1;
}


int LuaBindsAI::AI_SelectNearestTarget(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    float dist = luaL_checknumber(L, 2);
    int pos = luaL_checkinteger(L, 3);
    bool playerOnly = luaL_checkboolean(L, 4);
    int auraID = luaL_checkinteger(L, 5);
    lua_pushunitornil(L, ai->SelectTarget(SelectTargetMethod::MinDistance, pos, dist, false, 0));
    return 1;
}


int LuaBindsAI::AI_SelectPartyAttackTarget(lua_State* L) {
    LuaBotAI** ai = AI_GetAIObject(L);
    lua_pushunitornil(L, (*ai)->SelectPartyAttackTarget());
    return 1;
}


int LuaBindsAI::AI_SelectShieldTarget(lua_State* L) {
    LuaBotAI** ai = AI_GetAIObject(L);
    float hpRate = luaL_checknumber(L, 2);
    lua_pushplayerornil(L, (*ai)->SelectShieldTarget(hpRate));
    return 1;
}


// -----------------------------------------------------------
//                Spell Management RELATED
// -----------------------------------------------------------


int LuaBindsAI::AI_GetSpellChainFirst(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    int spellID = luaL_checkinteger(L, 2);
    uint32 result = ai->GetSpellChainFirst(spellID);
    if (result == 0)
        luaL_error(L, "AI.GetSpellChainFirst: spell not found. %d", spellID);
    lua_pushinteger(L, result);
    return 1;
}


int LuaBindsAI::AI_GetSpellChainLast(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    int spellID = luaL_checkinteger(L, 2);
    uint32 result = ai->GetSpellChainLast(spellID);
    if (result == 0)
        luaL_error(L, "AI.GetSpellChainLast: spell not found. %d", spellID);
    lua_pushinteger(L, result);
    return 1;
}


int LuaBindsAI::AI_GetSpellChainNext(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    int spellID = luaL_checkinteger(L, 2);
    uint32 result = ai->GetSpellChainNext(spellID);
    if (result == 0)
        luaL_error(L, "AI.GetSpellChainNext: spell not found. %d", spellID);
    lua_pushinteger(L, result);
    return 1;
}


int LuaBindsAI::AI_GetSpellChainPrev(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    int spellID = luaL_checkinteger(L, 2);
    uint32 result = ai->GetSpellChainPrev(spellID);
    if (result == 0)
        luaL_error(L, "AI.GetSpellChainPrev: spell not found. %d", spellID);
    lua_pushinteger(L, result);
    return 1;
}


int LuaBindsAI::AI_GetSpellMaxRankForLevel(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    int spellID = luaL_checkinteger(L, 2);
    int level = luaL_checkinteger(L, 3);
    uint32 result = ai->GetSpellMaxRankForLevel(spellID, level);
    // this could be an error
    if (result == 0 && !sSpellMgr->GetSpellInfo(spellID))
        luaL_error(L, "AI.GetSpellMaxRankForLevel: spell doesn't exist. %d", spellID);
    lua_pushinteger(L, result);
    return 1;
}


int LuaBindsAI::AI_GetSpellMaxRankForMe(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    int spellID = luaL_checkinteger(L, 2);
    uint32 result = ai->GetSpellMaxRankForMe(spellID);
    // this could be an error
    if (result == 0 && !sSpellMgr->GetSpellInfo(spellID))
        luaL_error(L, "AI.GetSpellMaxRankForMe: spell doesn't exist. %d", spellID);
    lua_pushinteger(L, result);
    return 1;
}


int LuaBindsAI::AI_GetSpellName(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    int spellID = luaL_checkinteger(L, 2);
    std::string result = ai->GetSpellName(spellID);
    if (result.size() == 0)
        luaL_error(L, "AI.GetSpellName: spell not found. %d", spellID);
    lua_pushstring(L, result.c_str());
    return 1;
}


int LuaBindsAI::AI_GetSpellOfRank(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    int spellID = luaL_checkinteger(L, 2);
    int rank = luaL_checkinteger(L, 3);
    uint32 result = ai->GetSpellOfRank(spellID, rank);
    if (result == 0)
        luaL_error(L, "AI.GetSpellOfRank: error, check logs. %d", spellID);
    lua_pushinteger(L, result);
    return 1;
}


int LuaBindsAI::AI_GetSpellRank(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    int spellID = luaL_checkinteger(L, 2);
    uint32 result = ai->GetSpellRank(spellID);
    if (result == 0)
        luaL_error(L, "AI.GetSpellRank: spell not found. %d", spellID);
    lua_pushinteger(L, result);
    return 1;
}


int LuaBindsAI::AI_GiveAllTalents(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);

    // from cs_learn.cpp
    Player* player = ai->me;
    uint32 classMask = player->getClassMask();

    for (uint32 i = 0; i < sTalentStore.GetNumRows(); ++i)
    {
        TalentEntry const* talentInfo = sTalentStore.LookupEntry(i);
        if (!talentInfo)
            continue;

        TalentTabEntry const* talentTabInfo = sTalentTabStore.LookupEntry(talentInfo->TalentTab);
        if (!talentTabInfo)
            continue;

        if ((classMask & talentTabInfo->ClassMask) == 0)
            continue;

        // xinef: search highest talent rank
        uint32 spellId = 0;
        uint8 rankId = MAX_TALENT_RANK;
        for (int8 rank = MAX_TALENT_RANK - 1; rank >= 0; --rank)
        {
            if (talentInfo->RankID[rank] != 0)
            {
                rankId = rank;
                spellId = talentInfo->RankID[rank];
                break;
            }
        }

        // xinef: some errors?
        if (!spellId || rankId == MAX_TALENT_RANK)
            continue;

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
        if (!spellInfo || !SpellMgr::IsSpellValid(spellInfo))
            continue;

        player->LearnTalent(talentInfo->TalentID, rankId, true);
    }

    player->SetFreeTalentPoints(0);
    player->SendTalentsInfoData(false);

    return 0;
}


int LuaBindsAI::AI_HasSpell(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    uint32 spellID = luaL_checkinteger(L, 2);
    if (!sSpellMgr->GetSpellInfo(spellID))
        luaL_error(L, "AI.HasSpell: spell doesn't exist %d", spellID);
    lua_pushboolean(L, ai->me->HasSpell(spellID));
    return 1;
}


int LuaBindsAI::AI_HasTalent(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    uint32 talentID = luaL_checkinteger(L, 2);

    TalentEntry const* talentInfo = sTalentStore.LookupEntry(talentID);
    if (!talentInfo)
        luaL_error(L, "AI.LearnTalent: talent doesn't exist %d", talentID);

    uint32 talentRank = luaL_checkinteger(L, 3);
    if (talentRank >= MAX_TALENT_RANK)
        luaL_error(L, "AI.LearnTalent: talent rank cannot exceed %d", MAX_TALENT_RANK - 1);

    TalentTabEntry const* talentTabInfo = sTalentTabStore.LookupEntry(talentInfo->TalentTab);
    if (!talentTabInfo)
        luaL_error(L, "AI.LearnTalent: talent tab not found for talent %d", talentID);

    uint32 classMask = ai->me->getClassMask();
    if ((classMask & talentTabInfo->ClassMask) == 0)
        luaL_error(L, "AI.LearnTalent: class mask and talent class mask do not match cls = %d, talent = %d", classMask, talentTabInfo->ClassMask);

    // search specified rank
    uint32 spellid = talentInfo->RankID[talentRank];
    if (!spellid)                                       // ??? none spells in talent
        luaL_error(L, "AI.LearnTalent: talent %d rank %d not found", talentID, talentRank);

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellid);
    if (!spellInfo || !SpellMgr::IsSpellValid(spellInfo))
        luaL_error(L, "AI.LearnTalent: talent %d spell %d is not valid for player or doesn't exist", talentID, spellid);

    lua_pushboolean(L, ai->me->HasTalent(spellid, ai->me->GetActiveSpec()));
    return 1;
}


int LuaBindsAI::AI_LearnSpell(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    uint32 spellID = luaL_checkinteger(L, 2);
    if (!sSpellMgr->GetSpellInfo(spellID))
        luaL_error(L, "AI.LearnSpell: spell doesn't exist %d", spellID);
    ai->me->learnSpell(spellID);
    return 0;
}


int LuaBindsAI::AI_LearnTalent(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    uint32 talentID = luaL_checkinteger(L, 2);
    if (!sTalentStore.LookupEntry(talentID))
        luaL_error(L, "AI.LearnTalent: talent doesn't exist %d", talentID);
    uint32 talentRank = luaL_checkinteger(L, 3);
    if (talentRank >= MAX_TALENT_RANK)
        luaL_error(L, "AI.LearnTalent: talent rank cannot exceed %d", MAX_TALENT_RANK - 1);
    bool force = luaL_checkboolean(L, 4);
    ai->me->LearnTalent(talentID, talentRank, force);
    return 0;
}


int LuaBindsAI::AI_GetTalentTbl(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);

    // from cs_learn.cpp
    Player* player = ai->me;
    uint32 classMask = player->getClassMask();

    lua_newtable(L);
    int tblIdx = 1;

    for (uint32 i = 0; i < sTalentStore.GetNumRows(); ++i)
    {
        TalentEntry const* talentInfo = sTalentStore.LookupEntry(i);
        if (!talentInfo)
            continue;

        TalentTabEntry const* talentTabInfo = sTalentTabStore.LookupEntry(talentInfo->TalentTab);
        if (!talentTabInfo)
            continue;

        if ((classMask & talentTabInfo->ClassMask) == 0)
            continue;

        // xinef: search highest talent rank
        uint32 spellId = 0;
        uint8 rankId = MAX_TALENT_RANK;
        for (int8 rank = MAX_TALENT_RANK - 1; rank >= 0; --rank)
        {
            if (talentInfo->RankID[rank] != 0)
            {
                rankId = rank;
                spellId = talentInfo->RankID[rank];
                break;
            }
        }

        // xinef: some errors?
        if (!spellId || rankId == MAX_TALENT_RANK)
            continue;

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
        if (!spellInfo || !SpellMgr::IsSpellValid(spellInfo))
            continue;

        lua_newtable(L);
        lua_newtable(L);
        for (lua_Integer rank = 0; rank <= rankId; ++rank)
            if (talentInfo->RankID[rank] != 0) {
                lua_pushinteger(L, talentInfo->RankID[rank]);
                lua_seti(L, -2, rank + 1);
            }
        lua_setfield(L, -2, "RankID");
        lua_pushinteger(L, spellId);
        lua_setfield(L, -2, "spellID");
        lua_pushinteger(L, rankId);
        lua_setfield(L, -2, "maxRankID");
        lua_pushinteger(L, talentInfo->TalentID);
        lua_setfield(L, -2, "TalentID");
        lua_pushinteger(L, talentInfo->Col);
        lua_setfield(L, -2, "Col");
        lua_pushinteger(L, talentInfo->Row);
        lua_setfield(L, -2, "Row");
        lua_pushinteger(L, talentInfo->addToSpellBook);
        lua_setfield(L, -2, "addToSpellBook");
        lua_pushinteger(L, talentInfo->TalentTab);
        lua_setfield(L, -2, "TalentTab");
        lua_pushinteger(L, talentInfo->DependsOn);
        lua_setfield(L, -2, "DependsOn");
        lua_pushinteger(L, talentInfo->DependsOnRank);
        lua_setfield(L, -2, "DependsOnRank");
        lua_pushstring(L, spellInfo->SpellName[0]);
        lua_setfield(L, -2, "spellName");
        lua_pushinteger(L, talentTabInfo->TalentTabID);
        lua_setfield(L, -2, "tabID");
        lua_seti(L, -2, tblIdx);
        ++tblIdx;
    }

    return 1;
}



int LuaBindsAI::AI_ResetTalents(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    lua_pushboolean(L, ai->me->resetTalents(true));
    return 1;
}



// -----------------------------------------------------------
//                      Equip RELATED
// -----------------------------------------------------------

int LuaBindsAI::AI_EquipCopyFromMaster(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    lua_pushboolean(L, ai->EquipCopyFromMaster());
    return 1;
}


int LuaBindsAI::AI_EquipItem(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    int itemID = luaL_checkinteger(L, 2);
    ai->EquipItem(itemID);
    return 0;
}


int LuaBindsAI::AI_EquipRandomGear(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    ai->EquipRandomGear();
    return 0;
}


int LuaBindsAI::AI_EquipDestroyAll(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    ai->EquipDestroyAll();
    return 0;
}


int LuaBindsAI::AI_EquipEnchant(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    int enchantID = luaL_checkinteger(L, 2);
    int islot = luaL_checkinteger(L, 3);
    int iitemSlot = luaL_checkinteger(L, 4);
    int duration = luaL_checkinteger(L, 5);
    int charges = luaL_checkinteger(L, 6);

    if (iitemSlot < EQUIPMENT_SLOT_START || iitemSlot >= EQUIPMENT_SLOT_END)
        luaL_error(L, "AI.EquipEnchant: Invalid equipment slot. Allowed values - [%d, %d). Got %d", EQUIPMENT_SLOT_START, EQUIPMENT_SLOT_END, iitemSlot);

    if (islot < PERM_ENCHANTMENT_SLOT || islot > MAX_ENCHANTMENT_SLOT)
        luaL_error(L, "AI.EquipEnchant: Invalid enchantment slot. Allowed values - [%d, %d). Got %d", PERM_ENCHANTMENT_SLOT, MAX_ENCHANTMENT_SLOT, islot);

    ai->EquipEnchant(enchantID, EnchantmentSlot(islot), EquipmentSlots(iitemSlot), duration, charges);
    return 0;
}


int LuaBindsAI::AI_EquipFindItemByName(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    std::string name(luaL_checkstring(L, 2));
    lua_pushinteger(L, ai->EquipFindItemByName(name));
    return 1;
}


int LuaBindsAI::AI_ClearGlyph(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    uint32 slot = luaL_checkinteger(L, 2);

    if (slot >= MAX_GLYPH_SLOT_INDEX)
        luaL_error(L, "AI.ClearGlyph: Glyph slot out of bounds. Available - [0, %d)", MAX_GLYPH_SLOT_INDEX);

    WorldPacket packet;
    packet.SetOpcode(CMSG_REMOVE_GLYPH);
    packet << slot;
    ai->me->GetSession()->HandleRemoveGlyph(packet);

    return 0;
}


int LuaBindsAI::AI_GetGlyph(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    uint32 slot = luaL_checkinteger(L, 2);
    if (slot >= MAX_GLYPH_SLOT_INDEX)
        luaL_error(L, "AI.GetGlyph: Glyph slot out of bounds. Available - [0, %d)", MAX_GLYPH_SLOT_INDEX);
    lua_pushinteger(L, ai->me->GetGlyph(slot));
    return 1;
}


int LuaBindsAI::AI_SetGlyph(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    uint32 slot = luaL_checkinteger(L, 2);
    uint32 glyph = luaL_checkinteger(L, 3);
    bool save = luaL_checkboolean(L, 4);

    if (slot >= MAX_GLYPH_SLOT_INDEX)
        luaL_error(L, "AI.SetGlyph: Glyph slot out of bounds. Available - [0, %d)", MAX_GLYPH_SLOT_INDEX);

    if (!sGlyphPropertiesStore.LookupEntry(glyph))
        luaL_error(L, "AI.SetGlyph: Glyph doesn't exist %d", glyph);

    if (ai->me->GetGlyph(slot)) {
        WorldPacket packet;
        packet.SetOpcode(CMSG_REMOVE_GLYPH);
        packet << slot;
        ai->me->GetSession()->HandleRemoveGlyph(packet);
    }

    if (GlyphPropertiesEntry const* glyphEntry = sGlyphPropertiesStore.LookupEntry(glyph))
        ai->me->CastSpell(ai->me, glyphEntry->SpellId, TriggerCastFlags(TRIGGERED_FULL_MASK & ~(TRIGGERED_IGNORE_SHAPESHIFT | TRIGGERED_IGNORE_CASTER_AURASTATE)));

    ai->me->SetGlyph(slot, glyph, save);
    return 0;
}

// -----------------------------------------------------------
//                      Pet RELATED
// -----------------------------------------------------------

int LuaBindsAI::AI_SummonPetIfNeeded(lua_State* L) {
    LuaBotAI** ai = AI_GetAIObject(L);
    uint32 petId = luaL_checkinteger(L, 2);
    (*ai)->SummonPetIfNeeded(petId);
    return 0;
}

int LuaBindsAI::AI_GetPet(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    lua_pushunitornil(L, ai->me->GetPet());
    return 1;
}

/// <summary>
/// If AI has pet sends attack command to pet ai.
/// </summary>
/// <param name="AI userdata">- AI.</param>
/// <param name="unit userdata">- Target to attack</param>
int LuaBindsAI::AI_PetAttack(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    Unit* pVictim = *Unit_GetUnitObject(L, 2);
    if (Pet* pPet = ai->me->GetPet())
        if (!pPet->GetVictim() || pPet->GetVictim()->GetGUID() != pVictim->GetGUID()) {
            WorldPacket p;
            p << pPet->GetGUID();
            p << MAKE_UNIT_ACTION_BUTTON(CommandStates::COMMAND_ATTACK, ActiveStates::ACT_COMMAND);
            p << pVictim->GetGUID();
            ai->me->GetSession()->HandlePetAction(p);
        }
    return 0;
}


/// <summary>
/// If AI has a pet, sets that pets autocast for spell to value of enable.
/// </summary>
/// <param name="unit userdata">- AI.</param>
/// <param name="spellID uint32">- spell id.</param>
/// <param name="enable bool">- enable autocast.</param>
int LuaBindsAI::AI_PetAutocast(lua_State* L) {

    LuaBotAI* ai = *AI_GetAIObject(L);
    uint32 spellID = luaL_checkinteger(L, 2);
    bool enable = luaL_checkboolean(L, 3);

    if (Pet* pPet = ai->me->GetPet()) {

        const SpellInfo* spell = sSpellMgr->GetSpellInfo(spellID);
        if (!spell)
            luaL_error(L, "AI.PetAutocast: spell doesn't exist %d", spellID);

        pPet->ToggleAutocast(spell, enable);
        if (pPet->GetCharmInfo())
            pPet->GetCharmInfo()->SetSpellAutocast(spell, enable);

    }

    return 0;

}

/// <summary>
/// If AI has pet tries to make it cast an ability
/// </summary>
/// <param name="AI userdata">- Pet owner</param>
/// <param name="unit userdata">- Target of spell</param>
/// <param name="int">- Spell ID</param>
/// <returns>int - Spell cast result</returns>
int LuaBindsAI::AI_PetCast(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    Unit* pVictim = *Unit_GetUnitObject(L, 2);
    uint32 spellId = luaL_checkinteger(L, 3);
    if (Pet* pPet = ai->me->GetPet())
        if (CreatureAI* pAI = pPet->AI()) {
            lua_pushinteger(L, pAI->DoCast(pVictim, spellId));
            return 1;
        }
    lua_pushinteger(L, -1);
    return 1;
}


/// <summary>
/// If unit has pet makes pet follow.
/// </summary>
/// <param name="unit userdata">- Pet owner</param>
int LuaBindsAI::AI_PetFollow(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    if (Pet* pPet = ai->me->GetPet()) {
        WorldPacket p;
        p << pPet->GetGUID();
        p << MAKE_UNIT_ACTION_BUTTON(CommandStates::COMMAND_FOLLOW, ActiveStates::ACT_COMMAND);
        p << ObjectGuid::Empty;
        ai->me->GetSession()->HandlePetAction(p);
    }
    return 0;
}


/// <summary>
/// If unit has pet issues pet stay command.
/// </summary>
/// <param name="unit userdata">- Pet owner</param>
int LuaBindsAI::AI_PetStay(lua_State* L) {
    LuaBotAI* ai = *AI_GetAIObject(L);
    if (Pet* pPet = ai->me->GetPet()) {
        WorldPacket p;
        p << pPet->GetGUID();
        p << MAKE_UNIT_ACTION_BUTTON(CommandStates::COMMAND_STAY, ActiveStates::ACT_COMMAND);
        p << ObjectGuid::Empty;
        ai->me->GetSession()->HandlePetAction(p);
    }
    return 0;
}

// -----------------------------------------------------------
//                      Movement RELATED
// -----------------------------------------------------------


int LuaBindsAI::AI_GoName(lua_State* L) {
    LuaBotAI** ai = AI_GetAIObject(L);
    char name[128] = {};
    strcpy(name, luaL_checkstring(L, 2));
    if (Player* dest = ObjectAccessor::FindPlayerByName(name))
        (*ai)->GoPlayerCommand(dest);
    return 0;
}


int LuaBindsAI::AI_Mount(lua_State* L) {
    LuaBotAI** ai = AI_GetAIObject(L);
    bool toMount = luaL_checkboolean(L, 2);
    uint32 mountSpell = luaL_checknumber(L, 3);
    (*ai)->Mount(toMount, mountSpell);
    return 0;
}


// -----------------------------------------------------------
//                    XP / LEVEL RELATED
// -----------------------------------------------------------


int LuaBindsAI::AI_InitTalentForLevel(lua_State* L) {
    LuaBotAI** ai = AI_GetAIObject(L);
    (*ai)->me->InitTalentForLevel();
    return 0;
}


int LuaBindsAI::AI_GiveLevel(lua_State* L) {
    LuaBotAI** ai = AI_GetAIObject(L);
    int level = luaL_checkinteger(L, 2);
    (*ai)->me->GiveLevel(level);
    (*ai)->me->InitTalentForLevel();
    (*ai)->me->SetUInt32Value(PLAYER_XP, 0);
    (*ai)->me->UpdateSkillsToMaxSkillsForLevel();
    (*ai)->me->SetFullHealth();
    Powers power = (*ai)->me->getPowerType();
    (*ai)->me->SetPower(power, (*ai)->me->GetMaxPower(power));
    return 0;
}


int LuaBindsAI::AI_SetXP(lua_State* L) {
    LuaBotAI** ai = AI_GetAIObject(L);
    int value = luaL_checkinteger(L, 2);
    (*ai)->me->SetUInt32Value(PLAYER_XP, value);
    return 0;
}


// -----------------------------------------------------------
//                    DEATH RELATED
// -----------------------------------------------------------


int LuaBindsAI::AI_ShouldAutoRevive(lua_State* L) {
    LuaBotAI** ai = AI_GetAIObject(L);
    lua_pushboolean(L, (*ai)->ShouldAutoRevive());
    return 1;
}







