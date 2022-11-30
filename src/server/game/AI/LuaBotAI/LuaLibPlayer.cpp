#include "LuaLibPlayer.h"
#include "LuaLibWorldObj.h"
#include "LuaLibUnit.h"
#include "LuaUtils.h"
#include "Group.h"
#include "Player.h"
#include "LuaBotAI.h"


void LuaBindsAI::Player_CreateUD(Player* player, lua_State* L) {
	// create userdata on top of the stack pointing to a pointer of an AI object
	Player** playerud = static_cast<Player**>(lua_newuserdatauv(L, sizeof(Player*), 0));
	*playerud = player; // swap the AI object being pointed to to the current instance
	luaL_setmetatable(L, LuaBindsAI::PlayerMtName);
}


void LuaBindsAI::BindPlayer(lua_State* L) {
	Player_CreateMetatable(L);
}


Player** LuaBindsAI::Player_GetPlayerObject(lua_State* L, int idx) {
	return (Player**) luaL_checkudata(L, idx, LuaBindsAI::PlayerMtName);
}


int LuaBindsAI_Player_CompareEquality(lua_State* L) {
	WorldObject* obj1 = *LuaBindsAI::WObj_GetWObjObject(L);
	WorldObject* obj2 = *LuaBindsAI::WObj_GetWObjObject(L, 2);
	lua_pushboolean(L, obj1 == obj2);
	return 1;
}
void LuaBindsAI::Player_CreateMetatable(lua_State* L) {
	luaL_newmetatable(L, LuaBindsAI::PlayerMtName);
	lua_pushvalue(L, -1); // copy mt cos setfield pops
	lua_setfield(L, -1, "__index"); // mt.__index = mt
	luaL_setfuncs(L, Unit_BindLib, 0); // copy funcs
	luaL_setfuncs(L, Player_BindLib, 0);
	lua_pushcfunction(L, LuaBindsAI_Player_CompareEquality);
	lua_setfield(L, -2, "__eq");
	lua_pushboolean(L, true);
	lua_setfield(L, -2, "isWorldObject");
	lua_pushboolean(L, true);
	lua_setfield(L, -2, "isUnit");
	lua_pop(L, 1); // pop mt
}


int LuaBindsAI::Player_InBattleGround(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);
    lua_pushboolean(L, player->InBattleground());
    return 1;
}

// ===================================================
// 0. Inventory
// ===================================================

int LuaBindsAI::Player_AddItem(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);
    uint32 id = luaL_checkinteger(L, 2);
    uint32 count = luaL_checkinteger(L, 3);
    if (player->AddItem(id, count)) {
        lua_pushboolean(L, true);
        return 1;
    }
    lua_pushboolean(L, false);
    return 1;
}


int LuaBindsAI::Player_EquipItem(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);
    uint32 id = luaL_checkinteger(L, 2);
    uint32 enchantId = luaL_checkinteger(L, 3);

    const ItemTemplate* pProto = sObjectMgr->GetItemTemplate(id);
    if (pProto) {

        if (pProto->RequiredLevel > player->getLevel()) {
            lua_pushboolean(L, false);
            return 1;
        }

        uint32 slot = player->FindEquipSlot(pProto, NULL_SLOT, true);
        if (slot != NULL_SLOT)
            if (Item* pItem2 = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                player->DestroyItem(INVENTORY_SLOT_BAG_0, slot, true);

        SatisfyItemRequirements(player, pProto);
        lua_pushboolean(L, player->StoreNewItemInBestSlots(pProto->ItemId, 1));
        return 1;

    }
    lua_pushboolean(L, false);
    return 1;
}





// ===================================================
// 1. Group
// ===================================================


int LuaBindsAI::Player_GetPartyLeader(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);
    Player* leader = nullptr;

    if (Group* g = player->GetGroup())
        leader = g->GetLeader();

    lua_pushplayerornil(L, leader);
    return 1;
}


// Builds a 1d table of all group attackers
int LuaBindsAI::Player_GetGroupAttackersTbl(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);
    lua_newtable(L);
    int tblIdx = 1;
    Group* pGroup = player->GetGroup();
    //printf("Fetching attacker tbl\n");
    for (GroupReference* itr = pGroup->GetFirstMember(); itr != nullptr; itr = itr->next())
        if (Player* pMember = itr->GetSource()) {

            //if (pMember->GetObjectGuid() == player->GetObjectGuid()) continue;
            for (const auto pAttacker : pMember->getAttackers())
                if (IsValidHostileTarget(pMember, pAttacker)) {
                    //printf("Additing target to tbl %s attacking %s\n", pAttacker->GetName(), pMember->GetName());
                    Unit_CreateUD(pAttacker, L); // pushes pAttacker userdata on top of stack
                    lua_seti(L, -2, tblIdx); // stack[-2][tblIdx] = stack[-1], pops pAttacker
                    tblIdx++;
                }

        }
    return 1;
}


int LuaBindsAI::Player_GetGroupMemberCount(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);
    if (Group* grp = player->GetGroup())
        lua_pushinteger(L, grp->GetMembersCount());
    else
        lua_pushinteger(L, 1);
    return 1;
}


int LuaBindsAI::Player_GetGroupTank(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);
    Group* pGroup = player->GetGroup();
    //printf("Fetching attacker tbl\n");
    for (GroupReference* itr = pGroup->GetFirstMember(); itr != nullptr; itr = itr->next())
        if (Player* pMember = itr->GetSource())
            if (LuaBotAI* pAI = pMember->GetLuaAI())
                if (pAI->GetRole() == ROLE_TANK) {
                    Player_CreateUD(pMember, L);
                    return 1;
                }

    return 0;
}


// Builds a table of all group members userdatas
int LuaBindsAI::Player_GetGroupTbl(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);
    lua_newtable(L);
    int tblIdx = 1;
    Group* pGroup = player->GetGroup();
    for (GroupReference* itr = pGroup->GetFirstMember(); itr != nullptr; itr = itr->next())
        if (Player* pMember = itr->GetSource()) {
            if (pMember == player) continue;
            Player_CreateUD(pMember, L);
            lua_seti(L, -2, tblIdx); // stack[1][tblIdx] = stack[-1], pops pMember
            tblIdx++;
        }
    return 1;
}


// Builds a 2d table of all group member threat lists
int LuaBindsAI::Player_GetGroupThreatTbl(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);
    lua_newtable(L);
    int tblIdx = 1;
    Group* pGroup = player->GetGroup();
    for (GroupReference* itr = pGroup->GetFirstMember(); itr != nullptr; itr = itr->next())
        if (Player* pMember = itr->GetSource()) {
            //if (pMember == player) continue;

            // main table
            lua_newtable(L);

            // player table
            int threatIdx = 2;
            Player_CreateUD(pMember, L);
            lua_seti(L, -2, 1); // playerTbl[1] = pMember, pops pMember
            for (auto v : pMember->GetThreatMgr().GetThreatList()) {
                if (Unit* hostile = v->GetSource()->GetOwner()) {
                    Unit_CreateUD(hostile, L);
                    lua_seti(L, -2, threatIdx); // playerTbl[threatIdx] = hostile, pops unit
                    threatIdx++;
                }
            }

            lua_seti(L, -2, tblIdx); // mainTbl[i]={...}, pops inner tbl
            tblIdx++;
        }
    return 1;
}


int LuaBindsAI::Player_IsGroupLeader(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);
    if (player->GetGroup() && player->GetGUID() == player->GetGroup()->GetLeaderGUID())
        lua_pushboolean(L, true);
    else
        lua_pushboolean(L, false);
    return 1;
}


int LuaBindsAI::Player_IsInGroup(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);
    if (player->GetGroup())
        lua_pushboolean(L, true);
    else
        lua_pushboolean(L, false);
    return 1;
}


int LuaBindsAI::Player_GetRole(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);
    if (LuaBotAI* pAI = player->GetLuaAI()) {
        lua_pushinteger(L, pAI->GetRole());
        return 1;
    }
    return 0;
}



// ===================================================
// 2. Death
// ===================================================

int LuaBindsAI::Player_BuildPlayerRepop(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);
    player->BuildPlayerRepop();
    return 0;
}


int LuaBindsAI::Player_ResurrectPlayer(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);
    int hp = luaL_checknumber(L, 2);
    bool bSick = luaL_checkboolean(L, 3);
    player->ResurrectPlayer(hp, bSick);
    return 0;
}


int LuaBindsAI::Player_RepopAtGraveyard(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);
    player->RepopAtGraveyard();
    return 0;
}


int LuaBindsAI::Player_SpawnCorpseBones(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);
    player->SpawnCorpseBones();
    return 0;
}





