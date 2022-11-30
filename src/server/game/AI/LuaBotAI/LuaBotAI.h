#ifndef _MANGOS_LUABOTAI_H_
#define _MANGOS_LUABOTAI_H_

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
class LuaBotAI {

    ShortTimeTracker m_updateTimer;

public:

    int logicID;
    Player* me; // changing this is a bad idea
    Player* master;
    bool ceaseUpdates;

    LuaBotAI(Player* me, Player* master, int logicID);
    ~LuaBotAI();

    void CeaseUpdates(bool cease = true) { ceaseUpdates = cease; }
    void Update(uint32 diff);

};

#endif
