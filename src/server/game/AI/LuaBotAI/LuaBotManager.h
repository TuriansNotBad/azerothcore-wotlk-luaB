#ifndef _MANGOS_LUABOTMANAGER_H_
#define _MANGOS_LUABOTMANAGER_H_

#include <unordered_map>

class Player;
class LuaBotLoginQueryHolder;

typedef std::unordered_map<ObjectGuid, Player*> LuaBotMap;

class LuaBotManager {

private:

    LuaBotMap m_bots;

public:
    LuaBotManager();
    ~LuaBotManager();

    static LuaBotManager& getInstance() {
        static LuaBotManager instance;
        return instance;
    }

    // no copy
    LuaBotManager(LuaBotManager const&) = delete;
    void operator=(LuaBotManager const&) = delete;

    // all about adding and removing bots

    void AddBot(const std::string& char_name, uint32 masterAccountId, int logicID);
    void HandlePlayerBotLoginCallback(LuaBotLoginQueryHolder const& holder);
    void LogoutPlayerBot(ObjectGuid guid);
    void LogoutAllBots();
    void OnBotLogin(Player* bot);

    Player* GetLuaBot(ObjectGuid guid);

    void Update(uint32 diff);

};

#define sLuaBotMgr LuaBotManager::getInstance()


#endif
