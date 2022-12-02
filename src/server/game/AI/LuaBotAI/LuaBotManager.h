#ifndef _MANGOS_LUABOTMANAGER_H_
#define _MANGOS_LUABOTMANAGER_H_

#include <unordered_map>

struct lua_State;
class Player;
class LuaBotLoginQueryHolder;
class WorldSession;

typedef std::unordered_map<ObjectGuid, Player*> LuaBotMap;

class LuaBotManager {

private:

    lua_State* L;
    LuaBotMap m_bots;

    bool bLuaCeaseUpdates;
    bool bLuaReload;

    bool LogoutPlayerBotInternal(ObjectGuid guid);

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

    // Getting bots into the game ***************************

    void AddBot(const std::string& char_name, uint32 masterAccountId, int logicID);
    void HandlePlayerBotLoginCallback(LuaBotLoginQueryHolder const& holder);
    void LogoutPlayerBot(ObjectGuid guid);
    void LogoutAllBots();
    void OnBotLogin(Player* bot);

    Player* GetLuaBot(ObjectGuid guid);

    // Lua basics *******************************************

    // returns current lua state
    lua_State* Lua() { return L; }

    /// <summary>
    /// Calls luaL_dofile. In case of errors logs, pops and ceases updates.
    /// </summary>
    /// <param name="filename">- file to do.</param>
    /// <returns>false in case of errors.</returns>
    bool LuaDofile(const std::string& filename);
    /// <summary>
    /// Closes lua_State if it's open and creates a new one.
    /// Reloads all lua files and cleans out all registered Logic/Goal data.
    /// </summary>
    void LuaLoadAll();
    /// <summary>
    /// Calls lua_dofile on all files in ai directory.
    /// </summary>
    void LuaLoadFiles(const std::string& fpath);
    /// <summary>
    /// Forces full Lua reload on next bot logic tick.
    /// All bot goals are reset, all registered logic and goal info is reset.
    /// All files reloaded.
    /// </summary>
    void LuaReload() { bLuaReload = true; }


    // Bot logic *********************************************

    void Update(uint32 diff);
    void HandleBotPackets(WorldSession* session);
    void UpdateSessions();

};

#define sLuaBotMgr LuaBotManager::getInstance()


#endif
