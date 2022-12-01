#ifndef MANGOS_LuaLibAI_H
#define MANGOS_LuaLibAI_H

#include "lua.hpp"
class LuaBotAI;

namespace LuaBindsAI {

	// Creates metatable for the AI userdata with name specified by AI::AI_MTNAME
	void BindAI(lua_State* L);
	// Creates metatable for the AI userdata with name specified by AI::AI_MTNAME.
	// Registers all the functions listed in LuaBindsBot::AI_BindLib with that metatable.
	void AI_CreateMetatable(lua_State* L);
	LuaBotAI** AI_GetAIObject(lua_State* L);
	int AI_AddTopGoal(lua_State* L);
	int AI_HasTopGoal(lua_State* L);

	int AI_Print(lua_State* L);
    int AI_GetUserTbl(lua_State* L);
    int AI_GetPlayer(lua_State* L);

    int AI_DrinkAndEat(lua_State* L);
    // combat related
    int AI_AddAmmo(lua_State* L);
    int AI_AttackAutoshot(lua_State* L);
    int AI_AttackStart(lua_State* L);
    int AI_AttackSet(lua_State* L);
    int AI_AttackSetChase(lua_State* L);
    int AI_AttackStop(lua_State* L);
    int AI_AttackStopAutoshot(lua_State* L);
    int AI_GetChaseDist(lua_State* L);
    int AI_GetChaseAngle(lua_State* L);
    int AI_GetChaseTarget(lua_State* L);
    int AI_UpdateChaseDist(lua_State* L);
    int AI_UpdateChaseAngle(lua_State* L);

    int AI_CanTryToCastSpell(lua_State* L);
    int AI_DoCastSpell(lua_State* L);

    int AI_GetAttackersInRangeCount(lua_State* L);
    int AI_GetClass(lua_State* L);
    int AI_GetMarkedTarget(lua_State* L);
    int AI_GetRole(lua_State* L);
    int AI_GetGameTime(lua_State* L);
    int AI_RunAwayFromTarget(lua_State* L);
    int AI_SelectPartyAttackTarget(lua_State* L);
    int AI_SelectNearestTarget(lua_State* L);
    int AI_SelectShieldTarget(lua_State* L);

    // pet related
    int AI_SummonPetIfNeeded(lua_State* L);
    int AI_GetPet(lua_State* L);
    int AI_PetAttack(lua_State* L);
    int AI_PetCast(lua_State* L);

    // movement related
    int AI_GoName(lua_State* L);
    int AI_Mount(lua_State* L);

    // xp/level related
    int AI_InitTalentForLevel(lua_State* L);
    int AI_GiveLevel(lua_State* L);
    int AI_SetXP(lua_State* L);

    // death related
    int AI_ShouldAutoRevive(lua_State* L);

    static const struct luaL_Reg AI_BindLib[]{
		{"AddTopGoal", AI_AddTopGoal},
		{"HasTopGoal", AI_HasTopGoal},

        {"GetUserTbl", AI_GetUserTbl},
        {"Print", AI_Print},
		{"GetPlayer", AI_GetPlayer},

        {"DrinkAndEat", AI_DrinkAndEat},
        // combat related
        {"AddAmmo", AI_AddAmmo},
        {"AttackAutoshot", AI_AttackAutoshot},
        {"AttackSet", AI_AttackSet},
        {"AttackSetChase", AI_AttackSetChase},
        {"AttackStart", AI_AttackStart},
        {"AttackStop", AI_AttackStop},
        {"AttackStopAutoshot", AI_AttackStopAutoshot},
        {"GetChaseDist", AI_GetChaseDist},
        {"GetChaseAngle", AI_GetChaseAngle},
        {"GetChaseTarget", AI_GetChaseTarget},
        {"UpdateChaseDist", AI_UpdateChaseDist},
        {"UpdateChaseAngle", AI_UpdateChaseAngle},


        {"CanTryToCastSpell", AI_CanTryToCastSpell},
        {"DoCastSpell", AI_DoCastSpell},
        {"GetAttackersInRangeCount", AI_GetAttackersInRangeCount},
        {"GetClass", AI_GetClass},
        {"GetMarkedTarget", AI_GetMarkedTarget},
        {"GetRole", AI_GetRole},
        {"GetGameTime", AI_GetGameTime},
        {"RunAwayFromTarget", AI_RunAwayFromTarget},
        {"SelectPartyAttackTarget", AI_SelectPartyAttackTarget},
        {"SelectNearestTarget", AI_SelectNearestTarget},
        {"SelectShieldTarget", AI_SelectShieldTarget},

        // pet related
        {"SummonPetIfNeeded", AI_SummonPetIfNeeded},
        {"GetPet", AI_GetPet},
        {"PetAttack", AI_PetAttack},
        {"PetCast", AI_PetCast},
        // {"Mount", AI_PetCasterChaseDist},

        // movement related
        {"GoName", AI_GoName},
        {"Mount", AI_Mount},

        // xp/level related
        {"GiveLevel",AI_GiveLevel},
        {"InitTalentForLevel", AI_InitTalentForLevel},
        {"SetXP", AI_SetXP},

        // death related
        {"ShouldAutoRevive", AI_ShouldAutoRevive},

        {NULL, NULL}
	};

}

#endif
