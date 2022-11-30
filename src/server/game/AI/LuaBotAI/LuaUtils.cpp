#include "LuaUtils.h"
#include "LuaLibPlayer.h"
#include "LuaLibUnit.h"
#include "Player.h"
#include "ReputationMgr.h"

bool luaL_checkboolean(lua_State* L, int idx) {
	bool result = false;
	if (lua_isboolean(L, idx))
		result = lua_toboolean(L, idx);
	else
		luaL_error(L, "Invalid argument %d type. Boolean expected, got %s", idx, lua_typename(L, lua_type(L, idx)));
	return result;
}


void lua_pushplayerornil(lua_State* L, Player* u) {
	if (u)
		LuaBindsAI::Player_CreateUD(u, L);
	else
		lua_pushnil(L);
}


void lua_pushunitornil(lua_State* L, Unit* u) {
	if (u)
		LuaBindsAI::Unit_CreateUD(u, L);
	else
		lua_pushnil(L);
}


void* luaL_checkudwithfield(lua_State* L, int idx, const char* fieldName) {
	void* p = lua_touserdata(L, idx);
	if (p != NULL)  /* value is a userdata? */
		if (lua_getmetatable(L, idx)) {  /* does it have a metatable? */
			if (lua_getfield(L, -1, fieldName))
				if (lua_toboolean(L, -1)) {
					lua_pop(L, 2);  /* remove metatable and field value */
					return p;
				}
			lua_pop(L, 2);  /* remove metatable and field value */
		}
	luaL_error(L, "Invalid argument type. Userdata expected, got %s", lua_typename(L, lua_type(L, idx)));
}


// copied from combatbotbaseai.cpp
bool LuaBindsAI::IsValidHostileTarget(Unit* me, Unit const* pTarget) {
	return me->IsValidAttackTarget(pTarget) &&
		pTarget->CanSeeOrDetect(me) &&
		!pTarget->HasBreakableByDamageCrowdControlAura() &&
		!pTarget->IsImmuneToAll() &&
		pTarget->GetTransport() == me->GetTransport();
}


void LuaBindsAI::SatisfyItemRequirements(Player* me, ItemTemplate const* pItem)
{
    if (me->getLevel() < pItem->RequiredLevel)
    {
        me->GiveLevel(pItem->RequiredLevel);
        me->InitTalentForLevel();
        me->SetUInt32Value(PLAYER_XP, 0);
    }

    // Set required reputation
    if (pItem->RequiredHonorRank)
        me->SetHonorPoints(sWorld->getIntConfig(CONFIG_MAX_HONOR_POINTS));

    if (pItem->RequiredReputationFaction && pItem->RequiredReputationRank)
        if (FactionEntry const* pFaction = sFactionStore.LookupEntry(pItem->RequiredReputationFaction))
            if (me->GetReputationMgr().GetRank(pFaction) < pItem->RequiredReputationRank)
                me->GetReputationMgr().SetReputation(pFaction, me->GetReputationMgr().Reputation_Cap);

    // Learn required spell
    if (pItem->RequiredSpell && !me->HasSpell(pItem->RequiredSpell))
        me->learnSpell(pItem->RequiredSpell, false, false);

    // Learn required profession
    if (pItem->RequiredSkill && (!me->HasSkill(pItem->RequiredSkill) || (me->GetSkillValue(pItem->RequiredSkill) < pItem->RequiredSkillRank)))
        me->SetSkill(pItem->RequiredSkill, pItem->RequiredSkillRank, me->GetMaxSkillValueForLevel(), me->GetMaxSkillValueForLevel());

    // Learn Dual Wield Specialization
    if (pItem->InventoryType == INVTYPE_WEAPONOFFHAND && !me->HasSpell(674))
        me->learnSpell(674, false, false);


}





