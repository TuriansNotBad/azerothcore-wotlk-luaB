#ifndef _MANGOS_LUABOTAI_H_
#define _MANGOS_LUABOTAI_H_

#include "GoalManager.h"
#include "LogicManager.h"
#include "UnitAI.h"

enum EnchantmentSlot;
enum EquipmentSlots;

enum LuaBotRole {
    ROLE_INVALID,
    ROLE_MDPS,
    ROLE_RDPS,
    ROLE_TANK,
    ROLE_HEALER
};

enum RaidTargetIcon : uint8
{
    RAID_TARGET_ICON_STAR = 0,
    RAID_TARGET_ICON_CIRCLE = 1,
    RAID_TARGET_ICON_DIAMOND = 2,
    RAID_TARGET_ICON_TRIANGLE = 3,
    RAID_TARGET_ICON_MOON = 4,
    RAID_TARGET_ICON_SQUARE = 5,
    RAID_TARGET_ICON_CROSS = 6,
    RAID_TARGET_ICON_SKULL = 7
};


// vmangos struct
struct ShortTimeTracker
{
    explicit ShortTimeTracker(int32 expiry = 0) : i_expiryTime(expiry) {}
    void Update(int32 diff) { i_expiryTime -= diff; }
    bool Passed() const { return i_expiryTime <= 0; }
    void Reset(int32 interval) { i_expiryTime = interval; }
    int32 GetExpiry() const { return i_expiryTime; }

private:
    int32 i_expiryTime;
};

class Player;
struct lua_State;
class LuaBotAI {

    lua_State* L;

    // time keeping

    ShortTimeTracker m_updateTimer;
    uint32 m_updateInterval;

    // registry refs

    int userDataRef;
    int userDataPlayerRef;
    int userTblRef;

    bool m_initialized;

    // managers

    Goal topGoal;
    LogicManager logicManager;
    GoalManager goalManager;

public:

    static const char* MTNAME;

    int logicID;
    int roleID;
    Player* me; // changing this is a bad idea
    Player* master;
    bool ceaseUpdates;

    LuaBotAI(Player* me, Player* master, int logicID);
    ~LuaBotAI();


    // AI userdata

    void CreateUD(lua_State* L);
    void PushUD(lua_State* L);
    int GetRef() { return userDataRef; }
    void SetRef(int n) { userDataRef = n; }
    void Unref(lua_State* L);

    // Player userdata

    void CreatePlayerUD(lua_State* L);
    void PushPlayerUD(lua_State* L);
    void UnrefPlayerUD(lua_State* L);


    // User table

    int GetUserTblRef() { return userTblRef; }
    void CreateUserTbl();
    void UnrefUserTbl(lua_State* L);

    // Top Goal

    Goal* AddTopGoal(int goalId, double life, std::vector<GoalParamP>& goalParams, lua_State* L);
    Goal* GetTopGoal() { return &topGoal; };

    // The actual logic

    bool IsInitalized() { return m_initialized; }
    bool IsReady();

    int GetRole() { return roleID; }
    void SetRole(int n) { roleID = n; }

    void CeaseUpdates(bool cease = true) { ceaseUpdates = cease; }
    void SetUpdateInterval(uint32 n) { m_updateInterval = n; }

    void HandleTeleportAck();

    void Init();
    void Update(uint32 diff);
    /// <summary>
    /// Resets bot to idle. Clears error flag. Removes shapeshift/mount auras.
    /// Lua state must be valid when called. Init must be called before next update.
    /// </summary>
    void Reset(bool dropRefs);

    // Vmangos ported partybots funcs

    void AttackAutoshot(Unit* pVictim, float chaseDist);
    void AttackStopAutoshot();
    void AddItemToInventory(uint32 itemId, uint32 count = 1);
    void AddAmmo();
    bool DrinkAndEat();
    void EquipRandomGear();
    uint8 GetAttackersInRangeCount(float range) const;
    Unit* GetMarkedTarget(RaidTargetIcon mark) const;
    uint32 GetPrimaryItemStatForClassAndRole(uint8 playerClass, uint8 role)
    {
        switch (playerClass)
        {
        case CLASS_DEATH_KNIGHT:
        case CLASS_WARRIOR:
        {
            return ITEM_MOD_STRENGTH;
        }
        case CLASS_PALADIN:
        {
            return ((role == ROLE_HEALER) ? ITEM_MOD_INTELLECT : ITEM_MOD_STRENGTH);
        }
        case CLASS_HUNTER:
        case CLASS_ROGUE:
        {
            return ITEM_MOD_AGILITY;
        }
        case CLASS_SHAMAN:
        case CLASS_DRUID:
        {
            return ((role == ROLE_MDPS || role == ROLE_TANK) ? ITEM_MOD_AGILITY : ITEM_MOD_INTELLECT);
        }
        case CLASS_PRIEST:
        case CLASS_MAGE:
        case CLASS_WARLOCK:
        {
            return ITEM_MOD_INTELLECT;
        }
        }
        return ITEM_MOD_STAMINA;
    }
    void GoPlayerCommand(Player* target);
    bool HandleSummonCommand(Player* target);
    void Mount(bool toMount, uint32 mountSpell);
    bool IsValidHostileTarget(Unit const* pTarget) const;
    bool IsValidDispelTarget(Unit* pTarget, SpellInfo const* pSpellEntry) const;
    bool MoveDistance(Unit* pTarget, float distance, float angle);
    bool RunAwayFromTarget(Unit* pTarget);
    Unit* SelectPartyAttackTarget() const;
    Player* SelectShieldTarget(float hpRate) const;
    bool ShouldAutoRevive() const;
    void SummonPetIfNeeded(uint32 petId);

    // Spell casting

    SpellCastResult DoCastSpell(Unit* pTarget, SpellInfo const* pSpellEntry);
    bool CanTryToCastSpell(Unit* pTarget, SpellInfo const* pSpellEntry, bool bAura = true) const;

    // UnitAI funcs

    // Select the best (up to) <num> targets (in <targetType> order) from the threat list that fulfill the following:
    // - Not among the first <offset> entries in <targetType> order (or SelectTargetMethod::MaxThreat order,
    //   if <targetType> is SelectTargetMethod::Random).
    // - Within at most <dist> yards (if dist > 0.0f)
    // - At least -<dist> yards away (if dist < 0.0f)
    // - Is a player (if playerOnly = true)
    // - Not the current tank (if withTank = false)
    // - Has aura with ID <aura> (if aura > 0)
    // - Does not have aura with ID -<aura> (if aura < 0)
    // The resulting targets are stored in <targetList> (which is cleared first).
    void SelectTargetList(std::list<Unit*>& targetList, uint32 num, SelectTargetMethod targetType, float dist, bool playerOnly, int32 aura);

    // Select the best (up to) <num> targets (in <targetType> order) satisfying <predicate> from the threat list and stores them in <targetList> (which is cleared first).
    // If <offset> is nonzero, the first <offset> entries in <targetType> order (or SelectTargetMethod::MaxThreat
    // order, if <targetType> is SelectTargetMethod::Random) are skipped.
    template <class PREDICATE>
    void SelectTargetList(std::list<Unit*>& targetList, PREDICATE const& predicate, uint32 maxTargets, SelectTargetMethod targetType);

    // Select the best target (in <targetType> order) from the threat list that fulfill the following:
    // - Not among the first <offset> entries in <targetType> order (or SelectTargetMethod::MaxThreat order,
    //   if <targetType> is SelectTargetMethod::Random).
    // - Within at most <dist> yards (if dist > 0.0f)
    // - At least -<dist> yards away (if dist < 0.0f)
    // - Is a player (if playerOnly = true)
    // - Not the current tank (if withTank = false)
    // - Has aura with ID <aura> (if aura > 0)
    // - Does not have aura with ID -<aura> (if aura < 0)
    Unit* SelectTarget(SelectTargetMethod targetType, uint32 position, float dist, bool playerOnly, int32 aura);

    // Select the best target (in <targetType> order) satisfying <predicate> from the threat list.
    // If <offset> is nonzero, the first <offset> entries in <targetType> order (or SelectTargetMethod::MaxThreat
    // order, if <targetType> is SelectTargetMethod::Random) are skipped.
    template <class PREDICATE>
    Unit* SelectTarget(SelectTargetMethod targetType, uint32 position, PREDICATE const& predicate);

    // fussing over your bots. equip. spells.
    uint32 GetSpellChainFirst(uint32 spellID);
    uint32 GetSpellChainLast(uint32 spellID);
    uint32 GetSpellChainPrev(uint32 spellID);
    uint32 GetSpellChainNext(uint32 spellID);
    uint32 GetSpellRank(uint32 spellID);
    uint32 GetSpellMaxRankForMe(uint32 firstSpell);
    uint32 GetSpellMaxRankForLevel(uint32 firstSpell, uint32 level);
    uint32 GetSpellOfRank(uint32 firstSpell, uint32 rank);
    std::string GetSpellName(uint32 spellID);

    uint32 EquipFindItemByName(const std::string& name);
    void EquipItem(uint32 itemID);
    void EquipDestroyAll();
    void EquipEnchant(uint32 enchantID, EnchantmentSlot slot, EquipmentSlots itemSlot, int duration, int charges);

    // Testing

    void Print();


};

#endif
