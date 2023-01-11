#include "LuaLibPlayer.h"
#include "LuaLibWorldObj.h"
#include "LuaLibUnit.h"
#include "LuaUtils.h"
#include "Group.h"
#include "Player.h"
#include "Pet.h"
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


int LuaBindsAI::Player_InBattleGroundQueue(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);
    bool ignoreArena = luaL_checkboolean(L, 2);
    lua_pushboolean(L, player->InBattlegroundQueue(ignoreArena));
    return 1;
}


int LuaBindsAI::Player_JoinBattleGroundQueue(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);
    int bg = luaL_checkinteger(L, 2);

    if (player->InBattlegroundQueue() || player->InBattleground())
        return 0;

    WorldPacket data(CMSG_BATTLEMASTER_JOIN);
    data << player->GetGUID();                       // battlemaster guid, or player guid if joining queue from BG portal
    data << uint32(bg);
    data << uint32(0);                                 // instance id, 0 if First Available selected
    data << uint8(0);                                  // join as group
    player->GetSession()->HandleBattlemasterJoinOpcode(data);

    return 0;
}


int LuaBindsAI::Player_JoinBattleGroundReviveQueue(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);
    int creatureType = luaL_checkinteger(L, 2);

    if (!player->InBattleground() || player->IsAlive())
        return 0;

    if (Battleground* battleground = player->GetBattleground(false)) {
        if (Creature* creature = battleground->GetBGCreature(creatureType)) {
            WorldPacket data(CMSG_AREA_SPIRIT_HEALER_QUEUE);
            data << creature->GetGUID();
            player->GetSession()->HandleAreaSpiritHealerQueueOpcode(data);
        }
    }

    return 0;
}


int LuaBindsAI::Player_GetBattleGroundTeam(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);

    if (!player->InBattleground()) {
        lua_pushinteger(L, -1);
        return 1;
    }

    lua_pushinteger(L, player->GetBgTeamId());
    return 1;
}


int LuaBindsAI::Player_GetBattleGroundStatus(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);

    if (!player->InBattleground()) {
        lua_pushinteger(L, -1);
        return 1;
    }

    if (Battleground* battleground = player->GetBattleground(false)) {
        lua_pushinteger(L, battleground->GetStatus());
        return 1;
    }

    lua_pushinteger(L, -1);
    return 1;
}


int LuaBindsAI::Player_IsInAreaTriggerRadius(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);
    int triggerId = luaL_checkinteger(L, 2);

    AreaTrigger const* areaTrigger = sObjectMgr->GetAreaTrigger(triggerId);

    if (!areaTrigger)
        luaL_error(L, "Player.IsInAreaTriggerRadius: %d trigger doesn't exist", triggerId);

    lua_pushboolean(L, player->IsInAreaTriggerRadius(areaTrigger));
    return 1;
}


int LuaBindsAI::Player_SendAreaTriggerPacket(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);
    int triggerId = luaL_checkinteger(L, 2);

    AreaTrigger const* areaTrigger = sObjectMgr->GetAreaTrigger(triggerId);

    if (!areaTrigger)
        luaL_error(L, "Player.SendAreaTriggerPacket: %d trigger doesn't exist", triggerId);

    WorldPacket data(CMSG_AREATRIGGER);
    data << uint32(triggerId);
    player->GetSession()->HandleAreaTriggerOpcode(data);
    return 0;
}


int LuaBindsAI::Player_TeleportTo(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);
    int mapID = luaL_checkinteger(L, 2);
    float x = luaL_checknumber(L, 3);
    float y = luaL_checknumber(L, 4);
    float z = luaL_checknumber(L, 5);


    if (mapID < 0)
        luaL_error(L, "Player.TeleportTo: Map ID cannot be less than 0. Got mapID=%d", mapID);

    if (player->IsBeingTeleported() || player->isBeingLoaded() || !player->IsInWorld()) {
        return 0;
    }

    // stop flight if need
    if (player->IsInFlight())
    {
        player->GetMotionMaster()->MovementExpired();
        player->CleanupAfterTaxiFlight();
    }
    // save only in non-flight case
    else {
        player->SaveRecallPosition();
    }
    
    // before GM
    lua_pushboolean(L, player->TeleportTo(mapID, x, y, z, player->GetOrientation(), 0));

    return 1;
}

// ===================================================
// 0. Inventory, AI, Spell
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

        if (pProto->RequiredLevel > player->GetLevel()) {
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


int LuaBindsAI::Player_GetAI(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);
    // nothing to give
    if (!player->IsLuaBot())
        return 0;
    LuaBotAI* ai = player->GetLuaAI();
    // weird
    if (!ai)
        return 0;
    ai->PushUD(L);
    return 1;
}


int LuaBindsAI::Player_IsLuaBot(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);
    lua_pushboolean(L, player->IsLuaBot());
    return 1;
}


int LuaBindsAI::Player_IsReady(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);
    // if bot check of bot initialized
    if (player->IsLuaBot() && !player->GetLuaAI()->IsInitalized()) {
        lua_pushboolean(L, false);
        return 1;
    }
    // is safe to use object
    if (!player->IsInWorld() || player->IsBeingTeleported() || player->isBeingLoaded()) {
        lua_pushboolean(L, false);
        return 1;
    }
    lua_pushboolean(L, true);
    return 1;
}


int LuaBindsAI::Player_HasSpell(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);
    uint32 spellID = luaL_checkinteger(L, 2);
    if (!sSpellMgr->GetSpellInfo(spellID))
        luaL_error(L, "Player.HasSpell: spell doesn't exist %d", spellID);
    lua_pushboolean(L, player->HasSpell(spellID));
    return 1;
}


int LuaBindsAI::Player_LearnSpell(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);
    uint32 spellID = luaL_checkinteger(L, 2);
    if (!sSpellMgr->GetSpellInfo(spellID))
        luaL_error(L, "Player.LearnSpell: spell doesn't exist %d", spellID);
    player->learnSpell(spellID);
    return 0;
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

    // no group, return empty table!
    if (!pGroup)
        return 1;

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

            if (Pet* pet = pMember->GetPet())
                for (const auto pAttacker : pet->getAttackers())
                    if (IsValidHostileTarget(pMember, pAttacker)) {
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

    // no group, cancel
    if (!pGroup)
        return 0;

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

    bool bots_only = false;
    bool exclude_self = true;
    bool safe_only = false;
    if (lua_gettop(L) > 1)
        bots_only = luaL_checkboolean(L, 2);

    if (lua_gettop(L) > 2)
        exclude_self = luaL_checkboolean(L, 3);

    if (lua_gettop(L) > 3)
        safe_only = luaL_checkboolean(L, 4);

    lua_newtable(L);
    int tblIdx = 1;
    Group* pGroup = player->GetGroup();

    // no group, return empty table!
    if (!pGroup)
        return 1;

    for (GroupReference* itr = pGroup->GetFirstMember(); itr != nullptr; itr = itr->next())
        if (Player* pMember = itr->GetSource()) {

            if (pMember == player && exclude_self) continue;
            if (bots_only && !pMember->IsLuaBot()) continue;

            if (safe_only) {
                if (!pMember->IsInWorld() || pMember->IsBeingTeleported() || pMember->isBeingLoaded())
                    continue;
                if (pMember->IsLuaBot() && !pMember->GetLuaAI()->IsInitalized())
                    continue;
            }
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

    // no group, return empty table!
    if (!pGroup)
        return 1;

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

int LuaBindsAI::Player_GetSubGroup(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);
    if (player->GetGroup())
        lua_pushinteger(L, player->GetSubGroup());
    else
        lua_pushinteger(L, 0);
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


int LuaBindsAI::Player_GetPet(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);
    lua_pushunitornil(L, player->GetPet());
    return 1;
}


// ===================================================
// 2. Death
// ===================================================

int LuaBindsAI::Player_BuildPlayerRepop(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);

    WorldLocation corpseLocation = player->GetCorpseLocation();
    if (player->GetCorpse() && corpseLocation.GetMapId() == player->GetMapId())
        return 0;

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


// ===================================================
// 3. Combat
// ===================================================


int LuaBindsAI::Player_RemoveSpellCooldown(lua_State* L) {
    Player* player = *Player_GetPlayerObject(L);
    uint32 spellId = luaL_checkinteger(L, 2);
    if (const SpellInfo* spell = sSpellMgr->GetSpellInfo(spellId))
        player->RemoveSpellCooldown(spellId);
    else
        luaL_error(L, "Unit.RemoveSpellCooldown spell doesn't exist. Id = %d", spellId);
    return 0;
}






