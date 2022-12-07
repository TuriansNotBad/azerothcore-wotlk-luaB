// login/logout code is mostly mod-playerbots code from https://github.com/ZhengPeiRu21/mod-playerbots

#include "lua.hpp"
#include "LuaBotManager.h"
#include "GroupMgr.h"
#include "Player.h"
#include "GameTime.h"
#include "Common.h"
#include "Group.h"
#include "QueryHolder.h"
#include "WorldSession.h"
#include "LuaBindsBotCommon.h"
#include <filesystem>

namespace fs = std::filesystem;


class LuaBotLoginQueryHolder : public LoginQueryHolder
{
private:
public:
    LuaBotLoginQueryHolder(uint32 accountId, ObjectGuid guid, Player* master, int logicID, std::string spec)
        : LoginQueryHolder(accountId, guid), master(master), logicID(logicID), spec(spec) { }
    Player* master;
    int logicID;
    std::string spec;
};


LuaBotManager::LuaBotManager() : L(nullptr), bLuaCeaseUpdates(false) {
    LuaLoadAll();
}

LuaBotManager::~LuaBotManager() {
    if (L) lua_close(L);
}

// ********************************************************
// **                  Lua basics                        **
// ********************************************************

bool LuaBotManager::LuaDofile(const std::string& filename) {
    if (luaL_dofile(L, filename.c_str()) != LUA_OK) {
        bLuaCeaseUpdates = true;
        LOG_ERROR("luabots", "Lua error executing file {}: {}\n", filename.c_str(), lua_tostring(L, -1));
        lua_pop(L, 1); // pop the error object
        return false;
    }
    return true;
}


void LuaBotManager::LuaLoadFiles(const std::string& fpath) {
    if (!L) return;

    // logic and goal list must be loaded before all other files.
    if (!LuaDofile((fpath + "/logic_list.lua")))
        return;
    if (!LuaDofile((fpath + "/goal_list.lua")))
        return;

    // do all files recursively
    for (const auto& entry : fs::recursive_directory_iterator(fpath))
        if (entry.path().extension().generic_string() == ".lua")
            if (!LuaDofile(entry.path().generic_string()))
                return;
}

void LuaBotManager::LuaLoadAll() {
    std::string fpath = "ai";
    if (L) lua_close(L); // kill old state
    L = nullptr;

    GoalManager::ClearGoalInfo();
    LogicManager::ClearLogicInfo();

    L = luaL_newstate();
    luaL_openlibs(L); // replace with individual libraries later

    lua_pushinteger(L, 0);
    lua_setglobal(L, "PB_LEADER_ROLE");
    LuaBindsAI::BindAll(L);

    LuaLoadFiles(fpath);
}

// ********************************************************
// **                  Bot Management                    **
// ********************************************************

Player* LuaBotManager::GetLuaBot(ObjectGuid guid) {
    LuaBotMap::const_iterator it = m_bots.find(guid);
    return (it == m_bots.end()) ? nullptr : it->second;
}


void LuaBotManager::AddBot(const std::string& char_name, uint32 masterAccountId, int logicID, std::string spec) {

    // find bot guid
    ObjectGuid botGuid = sCharacterCache->GetCharacterGuidByName(char_name);

    // already added
    Player* bot = ObjectAccessor::FindConnectedPlayer(botGuid);
    if (bot && bot->IsInWorld())
        return;

    // find bot account id
    uint32 accountId = sCharacterCache->GetCharacterAccountIdByGuid(botGuid);
    if (!accountId)
        return;


    if (WorldSession* masterSession = sWorld->FindSession(masterAccountId)) {

        if (!masterSession->GetPlayer())
            return;

        std::shared_ptr<LuaBotLoginQueryHolder> holder = std::make_shared<LuaBotLoginQueryHolder>(accountId, botGuid, masterSession->GetPlayer(), logicID, spec);
        if (!holder->Initialize()) {
            return;
        }

        masterSession->AddQueryHolderCallback(CharacterDatabase.DelayQueryHolder(holder)).AfterComplete([this](SQLQueryHolderBase const& holder) {
                HandlePlayerBotLoginCallback(static_cast<LuaBotLoginQueryHolder const&>(holder));
        });
    }
    
}


void LuaBotManager::HandlePlayerBotLoginCallback(LuaBotLoginQueryHolder const& holder) {

    uint32 botAccountId = holder.GetAccountId();

    WorldSession* botSession = new WorldSession(botAccountId, "", nullptr, SEC_PLAYER, EXPANSION_WRATH_OF_THE_LICH_KING, time_t(0), LOCALE_enUS, 0, false, false, 0);

    botSession->HandlePlayerLoginFromDB(holder); // will delete lqh

    Player* bot = botSession->GetPlayer();
    if (!bot)
    {
        LogoutPlayerBot(holder.GetGuid());
        LOG_ERROR("luabots", "Error logging in bot {}", holder.GetGuid().ToString().c_str());
        return;
    }

    bot->CreateLuaAI(bot, holder.master, holder.logicID);

    LuaBotAI* botAI = bot->GetLuaAI();
    if (!botAI) return;

    botAI->spec = holder.spec;

    OnBotLogin(bot);


}


void LuaBotManager::OnBotLogin(Player* bot) {

    m_bots[bot->GetGUID()] = bot;

    LuaBotAI* botAI = bot->GetLuaAI();
    if (!botAI) return;

    // botAI->Init();

}


bool LuaBotManager::LogoutPlayerBotInternal(ObjectGuid guid)
{
    if (Player* bot = GetLuaBot(guid)) {
        LOG_INFO("luabots", "Bot {} logging out", bot->GetName().c_str());
        bot->SaveToDB(false, false);

        WorldSession* botWorldSessionPtr = bot->GetSession();

        if (bot && !botWorldSessionPtr->isLogingOut()) {

            if (Group* g = bot->GetGroup())
                g->RemoveMember(bot->GetGUID(), RemoveMethod::GROUP_REMOVEMETHOD_KICK);

            botWorldSessionPtr->LogoutPlayer(true); // this will delete the bot Player object and PlayerbotAI object
            delete botWorldSessionPtr;              // finally delete the bot's WorldSession
            return true;
        }
    }
    return false;
}


void LuaBotManager::LogoutPlayerBot(ObjectGuid guid)
{
    if (LogoutPlayerBotInternal(guid))
        m_bots.erase(guid);
}


void LuaBotManager::LogoutAllBots() {

    for (auto itr = m_bots.begin(); itr != m_bots.end();)
        if (Player* bot = itr._Ptr->_Myval.second)

            if (LogoutPlayerBotInternal(bot->GetGUID()))
                itr = m_bots.erase(itr);                 // deletes bot player ptr inside this WorldSession PlayerBotMap
            else
                itr++;
            
        else
            itr = m_bots.erase(itr); // delete invalid bot entry

}


void LuaBotManager::GroupAll(Player* owner) {
    for (auto bot : m_bots) {

        if (!bot.second || !bot.second->IsInWorld() || bot.second->isBeingLoaded() || bot.second->IsBeingTeleported()) continue;

        if (LuaBotAI* botAI = bot.second->GetLuaAI()) {

            if (botAI->master && owner->GetGUID() == botAI->master->GetGUID()) {

                // if group exists invite
                if (Group * g = botAI->master->GetGroup()) {
                    if (g->GetMembersCount() > 4 && !g->isRaidGroup())
                        g->ConvertToRaid();
                    g->AddMember(bot.second);
                    g->BroadcastGroupUpdate();
                }
                else {
                    
                    // new group, delete on any error
                    Group * group = new Group;
                    if (owner->IsSpectator() || !group->Create(owner)) {
                        delete group;
                        return;
                    }

                    //if (!group->AddMember(owner)) {
                    //    delete group;
                    //    return;
                    //}
                    if (!group->AddMember(bot.second)) {
                        delete group;
                        return;
                    }
                    sGroupMgr->AddGroup(group);
                    group->BroadcastGroupUpdate();

                }
            }
        }
    }
}


void LuaBotManager::Update(uint32 diff) {

    // reload requested
    if (bLuaReload) {
        sWorld->SendServerMessage(SERVER_MSG_STRING, "Lua reload started");

        bLuaCeaseUpdates = false;
        bLuaReload = false;

        LuaLoadAll();

        // Code to reset each bot in the loop here
        for (auto bot : m_bots) {
            if (LuaBotAI* botAI = bot.second->GetLuaAI()) {
                // Reset bot movement, combat, goals.
                botAI->Reset(true);
                botAI->CeaseUpdates(false);
            }
        }
        sWorld->SendServerMessage(SERVER_MSG_STRING, "Lua reload finished");
    }

    // lua crashed on initialization
    if (bLuaCeaseUpdates)
        return;

    // make ai evaluate their thingdos
    for (auto bot : m_bots) {
        if (LuaBotAI* botAI = bot.second->GetLuaAI()) {
            botAI->Update(diff);
        }
    }

    UpdateSessions();

}

void LuaBotManager::UpdateSessions()
{
    for (auto itr : m_bots)
    {

        Player* const bot = itr.second;

        if (!bot || !bot->IsLuaBot())
            return;

        if (bot->IsBeingTeleported()) {
            bot->GetLuaAI()->HandleTeleportAck();
        }
        else if (bot->IsInWorld())
        {
            HandleBotPackets(bot->GetSession());
        }
    }
}

void LuaBotManager::HandleBotPackets(WorldSession* session)
{
    WorldPacket* packet;
    while (session->GetPacketQueue()->next(packet))
    {
        OpcodeClient opcode = static_cast<OpcodeClient>(packet->GetOpcode());
        ClientOpcodeHandler const* opHandle = opcodeTable[opcode];
        opHandle->Call(session, *packet);
        delete packet;
    }
}



