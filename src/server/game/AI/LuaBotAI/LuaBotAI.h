#ifndef _MANGOS_LUABOTAI_H_
#define _MANGOS_LUABOTAI_H_

#include "GoalManager.h"
#include "LogicManager.h"

enum LuaBotRole {
    ROLE_INVALID,
    ROLE_MDPS,
    ROLE_RDPS,
    ROLE_TANK,
    ROLE_HEALER
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

    int GetRole() { return roleID; }
    void SetRole(int n) { roleID = n; }

    void CeaseUpdates(bool cease = true) { ceaseUpdates = cease; }
    void SetUpdateInterval(uint32 n) { m_updateInterval = n; }

    void Init();
    void Update(uint32 diff);



    // Testing

    void Print();


};

#endif
