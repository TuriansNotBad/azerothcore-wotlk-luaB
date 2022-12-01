
#include "LuaBotAI.h"
#include "LuaLibPlayer.h"
#include "Player.h"
#include "Group.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "Pet.h"
#include "Chat.h"
#include "LuaBotManager.h"
#include "lua.hpp"


enum PartyBotSpells
{
    PB_SPELL_FOOD = 1131,
    PB_SPELL_DRINK = 1137,
    PB_SPELL_AUTO_SHOT = 75,
    PB_SPELL_SHOOT_WAND = 5019,
    PB_SPELL_HONORLESS_TARGET = 2479,
    // from combat bot
    SPELL_SUMMON_IMP = 688,
    SPELL_SUMMON_VOIDWALKER = 697,
    SPELL_SUMMON_FELHUNTER = 691,
    SPELL_SUMMON_SUCCUBUS = 712,
    SPELL_TAME_BEAST = 13481,
    SPELL_REVIVE_PET = 982,
    SPELL_CALL_PET = 883,
};


const char* LuaBotAI::MTNAME = "LuaObject.AI";

LuaBotAI::LuaBotAI(Player* me, Player* master, int logicID) :
    me(me),
    master(master),

    ceaseUpdates(false),
    m_updateInterval(50),

    userDataRef(LUA_NOREF),
    userDataPlayerRef(LUA_NOREF),
    userTblRef(LUA_NOREF),

    roleID(0),
    logicID(logicID),
    logicManager(logicID),
    topGoal(-1, 0, Goal::NOPARAMS, nullptr, nullptr, nullptr)
{
    m_updateTimer.Reset(2000);
    L = sLuaBotMgr.Lua();
    topGoal.SetTerminated(true);
}

LuaBotAI::~LuaBotAI() {
    Unref(L);
}


Goal* LuaBotAI::AddTopGoal(int goalId, double life, std::vector<GoalParamP>& goalParams, lua_State* L) {
    //topGoal.Unref(L);
    //topGoal.~Goal();
    topGoal = Goal(goalId, life, goalParams, &goalManager, L);
    goalManager.ClearActivationStack(); // top goal owns all of the goals, can nuke the entire manager
    goalManager.PushGoalOnActivationStack(&topGoal);
    return &topGoal;
}


void LuaBotAI::Init() {

    if (userDataRef == LUA_NOREF)
        CreateUD(L);
    if (userDataPlayerRef == LUA_NOREF)
        CreatePlayerUD(L);
    if (userTblRef == LUA_NOREF)
        CreateUserTbl();

    if (me->getLevel() != master->getLevel()) {
        me->GiveLevel(master->getLevel());
    }

    logicManager.Init(L, this);

}


void LuaBotAI::Update(uint32 diff) {
    
    // were instructed not to update; likely caused by lua error
    if (ceaseUpdates) return;

    // Is it time to update
    m_updateTimer.Update(diff);
    if (m_updateTimer.Passed())
        m_updateTimer.Reset(m_updateInterval);
    else
        return;

    // Did we corrupt the registry
    if (userDataRef == userDataPlayerRef || userDataRef == userTblRef || userTblRef == userDataPlayerRef) {
        LOG_ERROR("luabots", "LuaAI Core: Lua registry error... [{}, {}, {}]. Ceasing.\n", userDataRef, userDataPlayerRef, userTblRef);
        ceaseUpdates = true;
        return;
    }

    // bad pointers
    if (!me || !master) return;
    // hardcoded cease all logic ID
    if (logicID == -1) return;
    // we're not available for interactions
    if (!me->IsInWorld() || me->IsBeingTeleported() || me->isBeingLoaded())
        return;
    // master not available, do not update
    if (!master->IsInWorld() || master->IsBeingTeleported() || master->isBeingLoaded())
        return;
    // do not gain XP
    me->SetUInt32Value(PLAYER_XP, 0);
    // master in taxi?
    if (master->HasUnitState(UNIT_STATE_IN_FLIGHT)) {
        if (me->GetMotionMaster()->GetCurrentMovementGeneratorType()) {
            me->GetMotionMaster()->Clear(true);
            me->GetMotionMaster()->MoveIdle();
        }
        return;
    }

    // leave my group if master is not in it
    if (!me->InBattleground())
        if (Group* g = me->GetGroup())
            if (!g->IsMember(master->GetGUID()))
                g->Disband();

    // join group if invite
    if (me->GetGroupInvite()) {
        Group* group = me->GetGroupInvite();
        if (group->GetMembersCount() == 0)
            group->AddMember(group->GetLeader());
        group->AddMember(me);
        // group->SetLootMethod( LootMethod::GROUP_LOOT );
    }

    // maybe?
    if (me->GetTarget() == me->GetGUID())
        me->SetTarget();

    // let's gooo
    logicManager.Execute(L, this);
    if (!topGoal.GetTerminated()) {
        goalManager.Activate(L, this);
        goalManager.Update(L, this);
        goalManager.Terminate(L, this);
    }
    
    // one of the managers called error state
    if (ceaseUpdates) {
        goalManager = GoalManager();
        topGoal = Goal(0, 10.0, Goal::NOPARAMS, &goalManager, nullptr); // delete all goal objects
        topGoal.SetTerminated(true);
        // Reset AI as well to stop moving attacking, make it obivous there's an error...
        // Whisper master?
    }



}

// USER TABLE

void LuaBotAI::CreateUserTbl() {
    if (userTblRef == LUA_NOREF) {
        lua_newtable(L);
        userTblRef = luaL_ref(L, LUA_REGISTRYINDEX);
    }
}


void LuaBotAI::UnrefUserTbl(lua_State* L) {
    // unref already makes these checks but can't hurt.
    if (userTblRef != LUA_NOREF && userTblRef != LUA_REFNIL) {
        luaL_unref(L, LUA_REGISTRYINDEX, userTblRef);
        userTblRef = LUA_NOREF; // old ref no longer valid
    }
}

// AI USERDATA

void LuaBotAI::CreateUD(lua_State* L) {
    // create userdata on top of the stack pointing to a pointer of an AI object
    LuaBotAI** aiud = static_cast<LuaBotAI**>(lua_newuserdatauv(L, sizeof(LuaBotAI*), 0));
    *aiud = this; // swap the AI object being pointed to to the current instance
    luaL_setmetatable(L, MTNAME);
    // save this userdata in the registry table.
    userDataRef = luaL_ref(L, LUA_REGISTRYINDEX); // pops
}

void LuaBotAI::PushUD(lua_State* L) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, userDataRef);
}

void LuaBotAI::Unref(lua_State* L) {
    // unref already makes these checks but can't hurt.
    if (userDataRef != LUA_NOREF && userDataRef != LUA_REFNIL) {
        luaL_unref(L, LUA_REGISTRYINDEX, userDataRef);
        userDataRef = LUA_NOREF; // old ref no longer valid
    }
}

// PLAYER USERDATA

void LuaBotAI::CreatePlayerUD(lua_State* L) {
    LuaBindsAI::Player_CreateUD(me, L);
    // save this userdata in the registry table.
    userDataPlayerRef = luaL_ref(L, LUA_REGISTRYINDEX); // pops
}


void LuaBotAI::PushPlayerUD(lua_State* L) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, userDataPlayerRef);
}


void LuaBotAI::UnrefPlayerUD(lua_State* L) {
    if (userDataPlayerRef != LUA_NOREF && userDataPlayerRef != LUA_REFNIL) {
        luaL_unref(L, LUA_REGISTRYINDEX, userDataPlayerRef);
        userDataPlayerRef = LUA_NOREF; // old ref no longer valid
    }
}


// ========================================================================
// 1. Most/all of these functions are ports of Vmangos partybots functions
// ========================================================================

void LuaBotAI::AttackAutoshot(Unit* pVictim, float chaseDist) {
    me->Attack(pVictim, false);
    if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() == IDLE_MOTION_TYPE
        && me->GetDistance(pVictim) > chaseDist + 5)
    {
        me->GetMotionMaster()->MoveChase(pVictim, chaseDist);
    }

    if (me->HasSpell(PB_SPELL_AUTO_SHOT) &&
        (me->IsWithinCombatRange(pVictim, 5.0f) &&
        !me->IsNonMeleeSpellCast(false)))
    {
        switch (me->CastSpell(pVictim, PB_SPELL_AUTO_SHOT, false))
        {
        case SPELL_FAILED_NEED_AMMO:
        case SPELL_FAILED_NO_AMMO:
        {
            AddAmmo();
            me->CastSpell(pVictim, PB_SPELL_AUTO_SHOT, false);
            break;
        }
        }
    }
}

void LuaBotAI::AttackStopAutoshot() {
    if (me->GetCurrentSpell(CURRENT_AUTOREPEAT_SPELL)) {
        me->InterruptSpell(CURRENT_AUTOREPEAT_SPELL, true);
    }
}

bool LuaBotAI::DrinkAndEat()
{

    if (me->GetVictim())
        return false;

    bool const needToEat = me->GetHealthPct() < 50.0f;
    bool const needToDrink = (me->getPowerType() == POWER_MANA) && (me->GetPowerPct(POWER_MANA) < 50.0f);

    if (!needToEat && !needToDrink)
        return false;

    bool const isEating = me->HasAura(PB_SPELL_FOOD);
    bool const isDrinking = me->HasAura(PB_SPELL_DRINK);

    if (!isEating && needToEat)
    {
        if (me->GetMotionMaster()->GetCurrentMovementGeneratorType())
        {
            me->StopMoving();
            me->GetMotionMaster()->Clear(false);
            me->GetMotionMaster()->MoveIdle();
        }
        if (SpellInfo const* pSpellEntry = sSpellMgr->GetSpellInfo(PB_SPELL_FOOD))
        {
            me->CastSpell(me, pSpellEntry, true);
            me->RemoveSpellCooldown(PB_SPELL_FOOD);
        }
        return true;
    }

    if (!isDrinking && needToDrink)
    {
        if (me->GetMotionMaster()->GetCurrentMovementGeneratorType())
        {
            me->StopMoving();
            me->GetMotionMaster()->Clear(false);
            me->GetMotionMaster()->MoveIdle();
        }
        if (SpellInfo const* pSpellEntry = sSpellMgr->GetSpellInfo(PB_SPELL_DRINK))
        {
            me->CastSpell(me, pSpellEntry, true);
            me->RemoveSpellCooldown(PB_SPELL_DRINK);
        }
        return true;
    }

    return needToEat || needToDrink;
}

void LuaBotAI::AddItemToInventory(uint32 itemId, uint32 count)
{
    ItemPosCountVec dest;
    uint8 msg = me->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, count);
    if (msg == EQUIP_ERR_OK)
    {
        if (Item* pItem = me->StoreNewItem(dest, itemId, true, Item::GenerateItemRandomPropertyId(itemId)))
            pItem->SetCount(count);
    }
}

void LuaBotAI::AddAmmo()
{
    if (Item* pWeapon = me->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_RANGED))
    {
        if (ItemTemplate const* pWeaponProto = pWeapon->GetTemplate())
        {
            if (pWeaponProto->Class == ITEM_CLASS_WEAPON)
            {
                uint32 ammoType;
                switch (pWeaponProto->SubClass)
                {
                case ITEM_SUBCLASS_WEAPON_GUN:
                    ammoType = ITEM_SUBCLASS_BULLET;
                    break;
                case ITEM_SUBCLASS_WEAPON_BOW:
                case ITEM_SUBCLASS_WEAPON_CROSSBOW:
                    ammoType = ITEM_SUBCLASS_ARROW;
                    break;
                default:
                    return;
                }
                
                ItemTemplate const* pAmmoProto = nullptr;
                for (auto const& itr = sObjectMgr->GetItemTemplateStore()->begin(); itr != sObjectMgr->GetItemTemplateStore()->end();)
                {
                    
                    const ItemTemplate * pProto = &(itr->second);

                    if (pProto->Class == ITEM_CLASS_PROJECTILE &&
                        pProto->SubClass == ammoType &&
                        pProto->RequiredLevel <= me->getLevel() &&
                        (!pAmmoProto || pAmmoProto->ItemLevel < pProto->ItemLevel) &&
                        me->CanUseAmmo(pProto->ItemId) == EQUIP_ERR_OK)
                    {
                        pAmmoProto = pProto;
                    }
                    
                }

                if (pAmmoProto)
                {
                    if (Item* pItem = me->GetItemByPos(INVENTORY_SLOT_BAG_0, INVENTORY_SLOT_ITEM_START))
                        me->DestroyItem(INVENTORY_SLOT_BAG_0, INVENTORY_SLOT_ITEM_START, true);

                    AddItemToInventory(pAmmoProto->ItemId, pAmmoProto->GetMaxStackSize());
                    me->SetAmmo(pAmmoProto->ItemId);
                }
            }
        }
    }
}

bool LuaBotAI::CanTryToCastSpell(Unit* pTarget, SpellInfo const* pSpellEntry, bool bAura) const
{
    if (me->HasSpellCooldown(pSpellEntry->Id))
        return false;

    if (me->HasAuraType(AuraType::SPELL_AURA_MOD_SILENCE)
        && pSpellEntry->PreventionType == SpellPreventionType::SPELL_PREVENTION_TYPE_SILENCE)
        return false;

    if (me->GetGlobalCooldownMgr().HasGlobalCooldown(pSpellEntry))
        return false;
    
    if (pSpellEntry->TargetAuraState &&
        !pTarget->HasAuraState((AuraStateType) pSpellEntry->TargetAuraState))
        return false;

    if (pSpellEntry->CasterAuraState &&
        !me->HasAuraState((AuraStateType) pSpellEntry->CasterAuraState))
        return false;
    
    uint32 const powerCost = pSpellEntry->CalcPowerCost(me, pSpellEntry->GetSchoolMask());
    Powers const powerType = Powers(pSpellEntry->PowerType);

    if (powerType == POWER_HEALTH)
    {
        if (me->GetHealth() <= powerCost)
            return false;
        return true;
    }

    if (me->GetPower(powerType) < powerCost)
        return false;
    
    if (pTarget->IsImmunedToSpell(pSpellEntry))
        return false;
    
    if (pSpellEntry->CheckShapeshift(me->GetShapeshiftForm()) != SPELL_CAST_OK)
        return false;
    
    if (bAura && pSpellEntry->HasAnyAura() && pTarget->HasAura(pSpellEntry->Id))
        return false;

    SpellRangeEntry const* srange = pSpellEntry->RangeEntry;
    if (me != pTarget && pSpellEntry->Effects[0].GetImplicitTargetType() != EFFECT_IMPLICIT_TARGET_CASTER)
    {
        float maxR = pSpellEntry->GetMaxRange(pSpellEntry->IsPositive(), me);
        float minR = pSpellEntry->GetMinRange(pSpellEntry->IsPositive());

        if (!me->IsWithinCombatRange(pTarget, maxR) || me->IsWithinCombatRange(pTarget, minR))
            return false;
    }

    return true;
}

SpellCastResult LuaBotAI::DoCastSpell(Unit* pTarget, SpellInfo const* pSpellEntry)
{
    if (me != pTarget)
        me->SetFacingToObject(pTarget);
    
    if (me->IsMounted())
        me->RemoveAurasByType(AuraType::SPELL_AURA_MOUNTED);

    me->SetTarget(pTarget->GetGUID());
    //me->m_castingSpell = (me->GetClass() == CLASS_ROGUE) ? me->GetComboPoints() : pSpellEntry->Id;
    auto result = me->CastSpell(pTarget, pSpellEntry, false);

    //printf("cast %s result %u\n", pSpellEntry->SpellName[0].c_str(), result);

    // stop and retry
    if ((result == SPELL_FAILED_MOVING || result == SPELL_CAST_OK) &&
        (pSpellEntry->CalcCastTime(me) > 0) &&
        (me->isMoving() || !me->IsStopped()))
    {
        me->StopMoving();
        result = me->CastSpell(pTarget, pSpellEntry, false);
    }

    // give reagent and retry
    if ((result == SPELL_FAILED_NEED_AMMO_POUCH ||
        result == SPELL_FAILED_ITEM_NOT_READY) &&
        pSpellEntry->Reagent[0])
    {
        if (Item* pItem = me->GetItemByPos(INVENTORY_SLOT_BAG_0, INVENTORY_SLOT_ITEM_START))
            me->DestroyItem(INVENTORY_SLOT_BAG_0, INVENTORY_SLOT_ITEM_START, true);

        AddItemToInventory(pSpellEntry->Reagent[0]);
        result = me->CastSpell(pTarget, pSpellEntry, false);
    }


    return result;
}

uint8 LuaBotAI::GetAttackersInRangeCount(float range) const
{
    uint8 count = 0;
    for (const auto& pTarget : me->getAttackers())
        if (me->IsWithinCombatRange(pTarget, range))
            count++;

    return count;
}

Unit* LuaBotAI::GetMarkedTarget(RaidTargetIcon mark) const
{
    ObjectGuid targetGuid = me->GetGroup()->GetTargetWithIcon(mark);
    if (targetGuid.IsUnit())
        return ObjectAccessor::GetUnit(*me, targetGuid);

    return nullptr;
}

bool LuaBotAI::HandleSummonCommand(Player* target)
{
    if (!target)
        return false;

    if (target->GetGUID() == me->GetGUID())
        return false;

    return ChatHandler(target->GetSession()).ParseCommands("summon " + me->GetName());;
}

void LuaBotAI::GoPlayerCommand(Player* target) {
    // will crash if moving
    if (!me->IsStopped())
        me->StopMoving();
    me->AttackStop();
    me->GetMotionMaster()->Clear(false);
    me->GetMotionMaster()->MoveIdle();
    me->SetTarget();
    topGoal = Goal(0, 0, Goal::NOPARAMS, nullptr, nullptr);
    topGoal.SetTerminated(true);
    m_updateTimer.Reset(1000);
    HandleSummonCommand(target);
}

void LuaBotAI::Mount(bool toMount, uint32 mountSpell) {
    //Player* pLeader = GetPartyLeader();
    if (toMount && false == me->IsMounted())
    {

        // Leave shapeshift before mounting.
        if (me->IsInDisallowedMountForm() &&
            me->GetDisplayId() != me->GetNativeDisplayId() &&
            me->HasAuraType(SPELL_AURA_MOD_SHAPESHIFT))
            me->RemoveAurasByShapeShift();

        bool oldState = me->GetCommandStatus(CHEAT_CASTTIME);
        me->SetCommandStatusOn(CHEAT_CASTTIME);
        me->CastSpell(me, mountSpell, true);

        if (oldState)
            me->SetCommandStatusOn(CHEAT_CASTTIME);
        else
            me->SetCommandStatusOff(CHEAT_CASTTIME);

    }
    else if (!toMount && true == me->IsMounted())
        me->RemoveAurasByType(SPELL_AURA_MOUNTED);
}

bool LuaBotAI::IsValidHostileTarget(Unit const* pTarget) const
{
    return me->IsValidAttackTarget(pTarget) &&
        pTarget->CanSeeOrDetect(me) &&
        !pTarget->HasBreakableByDamageCrowdControlAura() &&
        !pTarget->IsImmuneToAll() &&
        pTarget->GetTransport() == me->GetTransport();
}

bool LuaBotAI::IsValidDispelTarget(Unit* pTarget, SpellInfo const* pSpellEntry) const
{
    uint32 dispelMask = 0;
    bool bFoundOneDispell = false;
    // Compute Dispel Mask
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (pSpellEntry->Effects[i].Effect != SPELL_EFFECT_DISPEL)
            continue;

        // Create dispel mask by dispel type
        dispelMask = pSpellEntry->GetDispelMask();
    }
    bool friendly_dispel = pTarget && pTarget->IsFriendlyTo(me);

    if (pTarget &&
        // Check immune for offensive dispel
        (!pTarget->IsImmunedToSchool(pSpellEntry) ||
            friendly_dispel))
    {
        if (!friendly_dispel && !me->IsValidAttackTarget(pTarget))
            return false;
        auto const& auras = pTarget->GetAppliedAuras();
        for (const auto& aura : auras)
        {

            AuraApplication* holder = aura.second;

            const SpellInfo* appliedAura = sSpellMgr->GetSpellInfo(holder->GetBase()->GetId());
            if (!appliedAura)
                continue;

            if ((1 << appliedAura->Dispel) & dispelMask)
            {
                if (appliedAura->Dispel == DISPEL_MAGIC ||
                    appliedAura->Dispel == DISPEL_DISEASE ||
                    appliedAura->Dispel == DISPEL_POISON)
                {
                    bool isCharm = pSpellEntry->HasAura(AuraType::SPELL_AURA_MOD_CHARM) || pSpellEntry->HasAura(AuraType::SPELL_AURA_AOE_CHARM);
                    bool positive = holder->IsPositive();
                    // do not remove positive auras if friendly target
                    // do not remove negative auras if non-friendly target
                    // when removing charm auras ignore hostile reaction from the charm
                    if (!friendly_dispel && !positive && isCharm)
                        if (Unit* charmer = pTarget->GetCharmer())
                            if (FactionTemplateEntry const* ft = charmer->GetFactionTemplateEntry())
                                if (FactionTemplateEntry const* ft2 = me->GetFactionTemplateEntry())
                                    if (ft->IsFriendlyTo(*ft2))
                                        bFoundOneDispell = true;
                    if (positive == friendly_dispel)
                        continue;
                }
                bFoundOneDispell = true;
                break;
            }
        }
    }

    if (!bFoundOneDispell)
        return false;

    return true;
}

bool LuaBotAI::MoveDistance(Unit* pTarget, float distance, float angle)
{
    if (me->GetTransport())
        return false;

    float x, y, z;
    pTarget->GetNearPoint(me, x, y, z, 0, distance, angle);

    if (z > (me->GetPositionZ() + 10.0f))
        return false;

    if (!pTarget->IsWithinLOS(x, y, z))
        return false;
    
    if (me->GetDistance(x, y, z) == 0.0f)
        return false;

    me->GetMotionMaster()->MovePoint(1111, x, y, z);

    return true;
}

bool LuaBotAI::RunAwayFromTarget(Unit* pTarget)
{
    Player* pLeader = nullptr;
    if (Group* g = me->GetGroup())
        pLeader = g->GetLeader();

    if (pLeader)
    {
        if (pLeader->IsInWorld() &&
            pLeader->GetMap() == me->GetMap())
        {
            float const distance = me->GetDistance(pLeader);
            if (distance >= 15.0f && distance <= 30.0f &&
                pLeader->GetDistance(pTarget) >= 15.0f)
            {
                me->MonsterMoveWithSpeed(pLeader->GetPositionX(), pLeader->GetPositionY(), pLeader->GetPositionZ(), me->GetSpeed(MOVE_RUN));
                return true;
            }
        }
    }

    float angle = pLeader ? me->GetAngle(pLeader) : me->GetAngle(pTarget);
    return MoveDistance(pTarget, 15.0f, angle);
}

Unit* LuaBotAI::SelectPartyAttackTarget() const
{
    Group* pGroup = me->GetGroup();
    for (GroupReference* itr = pGroup->GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        if (Player* pMember = itr->GetSource())
        {
            // We already checked self.
            if (pMember == me)
                continue;

            for (const auto pAttacker : pMember->getAttackers())
            {
                if (IsValidHostileTarget(pAttacker) &&
                    me->IsWithinDist(pAttacker, 50.0f))
                    return pAttacker;
            }
        }
    }

    return nullptr;
}

Player* LuaBotAI::SelectShieldTarget(float hpRate) const
{
    Group* pGroup = me->GetGroup();
    for (GroupReference* itr = pGroup->GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        if (Player* pMember = itr->GetSource())
        {
            bool shieldImmune = false;
            for (auto itr : pMember->GetAuraEffectsByType(AuraType::SPELL_AURA_MECHANIC_IMMUNITY))
                if (itr->GetMiscValue() == MECHANIC_SHIELD) {
                    shieldImmune = true;
                    break;
                }

            if ((pMember->GetHealthPct() < 90.0f) &&
                !pMember->getAttackers().empty() &&
                !shieldImmune)
                return pMember;
        }
    }

    return nullptr;
}

bool LuaBotAI::ShouldAutoRevive() const
{
    if (me->getDeathState() == DEAD)
        return true;

    bool alivePlayerNearby = false;
    Group* pGroup = me->GetGroup();
    for (GroupReference* itr = pGroup->GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        if (Player* pMember = itr->GetSource())
        {
            if (pMember == me)
                continue;

            if (pMember->IsInCombat())
                return false;

            if (pMember->IsAlive())
            {
                if (pMember->getLevel() > 13)
                    if (pMember->getClass() == CLASS_PRIEST ||
                        pMember->getClass() == CLASS_DRUID ||
                        pMember->getClass() == CLASS_PALADIN ||
                        pMember->getClass() == CLASS_SHAMAN)
                    return false;

                if (me->IsWithinDistInMap(pMember, 15.0f))
                    alivePlayerNearby = true;
            }
        }
    }

    return alivePlayerNearby;
}

void LuaBotAI::SummonPetIfNeeded(uint32 petId)
{
    if (me->getClass() == CLASS_HUNTER)
    {
        if (me->GetCharm())
            return;

        if (me->getLevel() < 10)
            return;

        if (me->GetPet())
        {
            if (Pet* pPet = me->GetPet())
            {
                if (!pPet->IsAlive()) {
                    uint32 mana = me->GetPower(POWER_MANA);
                    me->CastSpell(pPet, SPELL_REVIVE_PET, true);
                    me->SetPower(POWER_MANA, mana);
                }
            }
            else
                me->CastSpell(me, SPELL_CALL_PET, true);

            return;
        }

        if (Creature* pCreature = me->SummonCreature(petId,
            me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f,
            TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 3000))
        {
            pCreature->SetLevel(me->getLevel());
            me->CastSpell(pCreature, SPELL_TAME_BEAST, true);
        }
    }
}

// ========================================================================
// 2. UnitAI functions.
// ========================================================================

Unit* LuaBotAI::SelectTarget(SelectTargetMethod targetType, uint32 position, float dist, bool playerOnly, int32 aura)
{
    return SelectTarget(targetType, position, DefaultTargetSelector(me, dist, playerOnly, aura));
}

void LuaBotAI::SelectTargetList(std::list<Unit*>& targetList, uint32 num, SelectTargetMethod targetType, float dist, bool playerOnly, int32 aura)
{
    SelectTargetList(targetList, DefaultTargetSelector(me, dist, playerOnly, aura), num, targetType);
}

template <class PREDICATE>
Unit* LuaBotAI::SelectTarget(SelectTargetMethod targetType, uint32 position, PREDICATE const& predicate)
{
    ThreatContainer::StorageType const& threatlist = me->GetThreatMgr().GetThreatList();
    if (position >= threatlist.size())
        return nullptr;

    std::list<Unit*> targetList;
    for (ThreatContainer::StorageType::const_iterator itr = threatlist.begin(); itr != threatlist.end(); ++itr)
        if (predicate((*itr)->getTarget()))
            targetList.push_back((*itr)->getTarget());

    if (position >= targetList.size())
        return nullptr;

    if (targetType == SelectTargetMethod::MaxDistance || targetType == SelectTargetMethod::MinDistance)
        targetList.sort(Acore::ObjectDistanceOrderPred(me));

    switch (targetType)
    {
    case SelectTargetMethod::MaxDistance:
    case SelectTargetMethod::MaxThreat:
    {
        std::list<Unit*>::iterator itr = targetList.begin();
        std::advance(itr, position);
        return *itr;
    }
    case SelectTargetMethod::MinDistance:
    case SelectTargetMethod::MinThreat:
    {
        std::list<Unit*>::reverse_iterator ritr = targetList.rbegin();
        std::advance(ritr, position);
        return *ritr;
    }
    case SelectTargetMethod::Random:
    {
        std::list<Unit*>::iterator itr = targetList.begin();
        std::advance(itr, urand(position, targetList.size() - 1));
        return *itr;
    }
    default:
        break;
    }

    return nullptr;
}

template <class PREDICATE>
void LuaBotAI:: SelectTargetList(std::list<Unit*>& targetList, PREDICATE const& predicate, uint32 maxTargets, SelectTargetMethod targetType)
{
    ThreatContainer::StorageType const& threatlist = me->GetThreatMgr().GetThreatList();
    if (threatlist.empty())
        return;

    for (ThreatContainer::StorageType::const_iterator itr = threatlist.begin(); itr != threatlist.end(); ++itr)
        if (predicate((*itr)->getTarget()))
            targetList.push_back((*itr)->getTarget());

    if (targetList.size() < maxTargets)
        return;

    if (targetType == SelectTargetMethod::MaxDistance || targetType == SelectTargetMethod::MinDistance)
        targetList.sort(Acore::ObjectDistanceOrderPred(me));

    if (targetType == SelectTargetMethod::MinDistance || targetType == SelectTargetMethod::MinThreat)
        targetList.reverse();

    if (targetType == SelectTargetMethod::Random)
        Acore::Containers::RandomResize(targetList, maxTargets);
    else
        targetList.resize(maxTargets);
}



// TESTING

void LuaBotAI::Print() {
    printf("LuaBotAI object. Class = %d, userDataRef = %d\n", me->getClass(), userDataRef);
}






