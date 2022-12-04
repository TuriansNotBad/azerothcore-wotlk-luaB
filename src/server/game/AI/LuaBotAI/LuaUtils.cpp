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

    // proficiency
    if (pItem->GetSkill()) {
        uint32 skill = pItem->GetSkill();
        //if (skill == SKILL_MAIL)
            //me->learnSpell()
        me->SetSkill(skill, 1, me->GetMaxSkillValueForLevel(), me->GetMaxSkillValueForLevel());
    }

    // Learn required profession
    if (pItem->RequiredSkill && (!me->HasSkill(pItem->RequiredSkill) || (me->GetSkillValue(pItem->RequiredSkill) < pItem->RequiredSkillRank)))
        me->SetSkill(pItem->RequiredSkill, pItem->RequiredSkillRank, me->GetMaxSkillValueForLevel(), me->GetMaxSkillValueForLevel());

    // Learn Dual Wield Specialization
    if (pItem->InventoryType == INVTYPE_WEAPONOFFHAND && !me->HasSpell(674))
        me->learnSpell(674, false, false);


}

void LuaBindsAI::GetEquipSlotsForType(InventoryType type, uint32 SubClass, uint8 slots[4], uint8 classId, bool canDualWield)
{

    slots[0] = NULL_SLOT;
    slots[1] = NULL_SLOT;
    slots[2] = NULL_SLOT;
    slots[3] = NULL_SLOT;

    switch (type)
    {
    case INVTYPE_HEAD:
        slots[0] = EQUIPMENT_SLOT_HEAD;
        break;
    case INVTYPE_NECK:
        slots[0] = EQUIPMENT_SLOT_NECK;
        break;
    case INVTYPE_SHOULDERS:
        slots[0] = EQUIPMENT_SLOT_SHOULDERS;
        break;
    case INVTYPE_BODY:
        slots[0] = EQUIPMENT_SLOT_BODY;
        break;
    case INVTYPE_CHEST:
        slots[0] = EQUIPMENT_SLOT_CHEST;
        break;
    case INVTYPE_ROBE:
        slots[0] = EQUIPMENT_SLOT_CHEST;
        break;
    case INVTYPE_WAIST:
        slots[0] = EQUIPMENT_SLOT_WAIST;
        break;
    case INVTYPE_LEGS:
        slots[0] = EQUIPMENT_SLOT_LEGS;
        break;
    case INVTYPE_FEET:
        slots[0] = EQUIPMENT_SLOT_FEET;
        break;
    case INVTYPE_WRISTS:
        slots[0] = EQUIPMENT_SLOT_WRISTS;
        break;
    case INVTYPE_HANDS:
        slots[0] = EQUIPMENT_SLOT_HANDS;
        break;
    case INVTYPE_FINGER:
        slots[0] = EQUIPMENT_SLOT_FINGER1;
        slots[1] = EQUIPMENT_SLOT_FINGER2;
        break;
    case INVTYPE_TRINKET:
        slots[0] = EQUIPMENT_SLOT_TRINKET1;
        slots[1] = EQUIPMENT_SLOT_TRINKET2;
        break;
    case INVTYPE_CLOAK:
        slots[0] = EQUIPMENT_SLOT_BACK;
        break;
    case INVTYPE_WEAPON:
    {
        slots[0] = EQUIPMENT_SLOT_MAINHAND;

        // suggest offhand slot only if know dual wielding
        // (this will be replace mainhand weapon at auto equip instead unwonted "you don't known dual wielding" ...
        if (canDualWield)
            slots[1] = EQUIPMENT_SLOT_OFFHAND;
        break;
    };
    case INVTYPE_SHIELD:
        slots[0] = EQUIPMENT_SLOT_OFFHAND;
        break;
    case INVTYPE_RANGED:
        slots[0] = EQUIPMENT_SLOT_RANGED;
        break;
    case INVTYPE_2HWEAPON:
        slots[0] = EQUIPMENT_SLOT_MAINHAND;
        break;
    case INVTYPE_TABARD:
        slots[0] = EQUIPMENT_SLOT_TABARD;
        break;
    case INVTYPE_WEAPONMAINHAND:
        slots[0] = EQUIPMENT_SLOT_MAINHAND;
        break;
    case INVTYPE_WEAPONOFFHAND:
        slots[0] = EQUIPMENT_SLOT_OFFHAND;
        break;
    case INVTYPE_HOLDABLE:
        slots[0] = EQUIPMENT_SLOT_OFFHAND;
        break;
    case INVTYPE_THROWN:
        slots[0] = EQUIPMENT_SLOT_RANGED;
        break;
    case INVTYPE_RANGEDRIGHT:
        slots[0] = EQUIPMENT_SLOT_RANGED;
        break;
    case INVTYPE_BAG:
        slots[0] = INVENTORY_SLOT_BAG_START + 0;
        slots[1] = INVENTORY_SLOT_BAG_START + 1;
        slots[2] = INVENTORY_SLOT_BAG_START + 2;
        slots[3] = INVENTORY_SLOT_BAG_START + 3;
        break;
    case INVTYPE_RELIC:
    {
        switch (SubClass)
        {
        case ITEM_SUBCLASS_ARMOR_LIBRAM:
            if (classId == CLASS_PALADIN)
                slots[0] = EQUIPMENT_SLOT_RANGED;
            break;
        case ITEM_SUBCLASS_ARMOR_IDOL:
            if (classId == CLASS_DRUID)
                slots[0] = EQUIPMENT_SLOT_RANGED;
            break;
        case ITEM_SUBCLASS_ARMOR_TOTEM:
            if (classId == CLASS_SHAMAN)
                slots[0] = EQUIPMENT_SLOT_RANGED;
            break;
        case ITEM_SUBCLASS_ARMOR_SIGIL:
            if (classId == CLASS_DEATH_KNIGHT)
                slots[0] = EQUIPMENT_SLOT_RANGED;
            break;
        case ITEM_SUBCLASS_ARMOR_MISC:
            if (classId == CLASS_WARLOCK)
                slots[0] = EQUIPMENT_SLOT_RANGED;
            break;
        }
        break;
    }
    }
}


bool LuaBindsAI::IsShieldClass(uint8 playerClass)
{
    switch (playerClass)
    {
    case CLASS_WARRIOR:
    case CLASS_PALADIN:
    case CLASS_SHAMAN:
        return true;
    }
    return false;
}

uint32 LuaBindsAI::GetHighestKnownArmorProficiency(Player* me)
{
    if (me->GetSkillValue(SKILL_PLATE_MAIL))
        return SKILL_PLATE_MAIL;
    if (me->GetSkillValue(SKILL_MAIL))
        return SKILL_MAIL;
    if (me->GetSkillValue(SKILL_LEATHER))
        return SKILL_LEATHER;
    return SKILL_CLOTH;
}






