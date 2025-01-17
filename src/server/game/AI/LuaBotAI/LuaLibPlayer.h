#ifndef MANGOS_LuaLibPlayer_H
#define MANGOS_LuaLibPlayer_H

#include "lua.hpp"

namespace LuaBindsAI {
	static const char* PlayerMtName = "LuaAI.Player";

	// Creates metatable for the AI userdata with name specified by AI::Unit_MTNAME
	void BindPlayer(lua_State* L);
	// Creates metatable for the AI userdata with name specified by AI::Unit_MTNAME.
	// Registers all the functions listed in LuaBindsBot::Unit_BindLib with that metatable.
	void Player_CreateMetatable(lua_State* L);
	Player** Player_GetPlayerObject(lua_State* L, int idx = 1);
	void Player_CreateUD(Player* player, lua_State* L);

    // battlegrounds

    int Player_InBattleGround(lua_State* L);
    int Player_InBattleGroundQueue(lua_State* L);
    int Player_JoinBattleGroundQueue(lua_State* L);
    int Player_GetBattleGroundTeam(lua_State* L);
    int Player_GetBattleGroundStatus(lua_State* L);
    int Player_JoinBattleGroundReviveQueue(lua_State* L);

    int Player_IsInAreaTriggerRadius(lua_State* L);
    int Player_SendAreaTriggerPacket(lua_State* L);
    int Player_TeleportTo(lua_State* L);

    //equip

    int Player_AddItem(lua_State* L);
    int Player_EquipItem(lua_State* L);

    int Player_GetComboPoints(lua_State* L);

    // spells

    int Player_LearnSpell(lua_State* L);
    int Player_HasSpell(lua_State* L);

    //ai

    int Player_GetAI(lua_State* L);
    int Player_IsLuaBot(lua_State* L);
    int Player_IsReady(lua_State* L);

    // party related
    int Player_GetGroupAttackersTbl(lua_State* L);
    int Player_GetGroupMemberCount(lua_State* L);
    int Player_GetGroupTank(lua_State* L);
    int Player_GetGroupTbl(lua_State* L);
    int Player_GetGroupThreatTbl(lua_State* L);
    int Player_GetPartyLeader(lua_State* L);
    int Player_GetSubGroup(lua_State* L);
    int Player_IsGroupLeader(lua_State* L);
    int Player_IsInGroup(lua_State* L);
    int Player_GetRole(lua_State* L);
    int Player_GetPet(lua_State* L);

    // death related
    int Player_BuildPlayerRepop(lua_State* L);
    int Player_RepopAtGraveyard(lua_State* L);
    int Player_ResurrectPlayer(lua_State* L);
    int Player_SpawnCorpseBones(lua_State* L);

    // death related
    int Player_RemoveSpellCooldown(lua_State* L);

	static const struct luaL_Reg Player_BindLib[]{

        // bgs
        {"InBattleGround", Player_InBattleGround},
        {"InBattleGroundQueue", Player_InBattleGroundQueue},
        {"JoinBattleGroundQueue", Player_JoinBattleGroundQueue},
        {"JoinBattleGroundReviveQueue", Player_JoinBattleGroundReviveQueue},
        {"GetBattleGroundTeam", Player_GetBattleGroundTeam},
        {"GetBattleGroundStatus", Player_GetBattleGroundStatus},

        {"IsInAreaTriggerRadius", Player_IsInAreaTriggerRadius},
        {"SendAreaTriggerPacket", Player_SendAreaTriggerPacket},
        {"TeleportTo", Player_TeleportTo},

        // equip
        {"AddItem", Player_AddItem},
        {"EquipItem", Player_EquipItem},

        {"GetComboPoints", Player_GetComboPoints},
        // spells
        {"LearnSpell", Player_LearnSpell},
        {"HasSpell", Player_HasSpell},

        // ai
        {"GetAI", Player_GetAI},
        {"IsLuaBot", Player_IsLuaBot},
        {"IsReady", Player_IsReady},

        // party related
        {"GetGroupAttackersTbl", Player_GetGroupAttackersTbl},
        {"GetGroupMemberCount", Player_GetGroupMemberCount},
        {"GetGroupTank", Player_GetGroupTank},
        {"GetGroupTbl", Player_GetGroupTbl},
        {"GetGroupThreatTbl", Player_GetGroupThreatTbl},
        {"GetPartyLeader", Player_GetPartyLeader},
        {"GetSubGroup", Player_GetSubGroup},
        {"IsGroupLeader", Player_IsGroupLeader},
        {"IsInGroup", Player_IsInGroup},
        {"GetRole", Player_GetRole},
        {"GetPet", Player_GetPet},

        // death related
        {"BuildPlayerRepop", Player_BuildPlayerRepop},
        {"RepopAtGraveyard", Player_RepopAtGraveyard},
        {"ResurrectPlayer", Player_ResurrectPlayer},
        {"SpawnCorpseBones", Player_SpawnCorpseBones},

        // Combat
        {"RemoveSpellCooldown", Player_RemoveSpellCooldown},

        {NULL, NULL}
	};


}

#endif
