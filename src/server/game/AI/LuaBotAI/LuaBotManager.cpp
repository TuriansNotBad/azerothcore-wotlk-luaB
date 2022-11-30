// mostly mod-playerbots code from https://github.com/ZhengPeiRu21/mod-playerbots

#include "LuaBotManager.h"
#include "Player.h"
#include "GameTime.h"
#include "Common.h"
#include "QueryHolder.h"
#include "WorldSession.h"


class LuaBotLoginQueryHolder : public LoginQueryHolder
{
private:
public:
    LuaBotLoginQueryHolder(uint32 accountId, ObjectGuid guid, Player* master, int logicID)
        : LoginQueryHolder(accountId, guid), master(master), logicID(logicID) { }
    Player* master;
    int logicID;
};


LuaBotManager::LuaBotManager() {

}

LuaBotManager::~LuaBotManager() {

}


Player* LuaBotManager::GetLuaBot(ObjectGuid guid) {
    LuaBotMap::const_iterator it = m_bots.find(guid);
    return (it == m_bots.end()) ? nullptr : it->second;
}


void LuaBotManager::AddBot(const std::string& char_name, uint32 masterAccountId, int logicID) {

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

        std::shared_ptr<LuaBotLoginQueryHolder> holder = std::make_shared<LuaBotLoginQueryHolder>(accountId, botGuid, masterSession->GetPlayer(), logicID);
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
        //LogoutPlayerBot(holder.GetGuid());
        LOG_ERROR("luabots", "Error logging in bot {}", holder.GetGuid().ToString().c_str());
        return;
    }

    bot->CreateLuaAI(bot, holder.master, holder.logicID);

    OnBotLogin(bot);
}


void LuaBotManager::OnBotLogin(Player* bot) {

    m_bots[bot->GetGUID()] = bot;

    LuaBotAI* botAI = bot->GetLuaAI();
    if (!botAI) return;

    if (bot->getLevel() != botAI->master->getLevel()) {
        bot->GiveLevel(botAI->master->getLevel());
    }

}


void LuaBotManager::LogoutPlayerBot(ObjectGuid guid)
{
    if (Player* bot = GetLuaBot(guid)) {
        LOG_INFO("luabots", "Bot {} logging out", bot->GetName().c_str());
        bot->SaveToDB(false, false);

        WorldSession* botWorldSessionPtr = bot->GetSession();

        if (bot && !botWorldSessionPtr->isLogingOut()) {
            m_bots.erase(guid);                 // deletes bot player ptr inside this WorldSession PlayerBotMap
            botWorldSessionPtr->LogoutPlayer(true); // this will delete the bot Player object and PlayerbotAI object
            delete botWorldSessionPtr;              // finally delete the bot's WorldSession
        }
    }
}


void LuaBotManager::LogoutAllBots() {

    for (auto itr = m_bots.begin(); itr != m_bots.end();) {
        // lazy code incoming
        if (Player* bot = itr._Ptr->_Myval.second) {
            LOG_INFO("luabots", "Bot {} logging out", bot->GetName().c_str());
            bot->SaveToDB(false, false);

            WorldSession* botWorldSessionPtr = bot->GetSession();

            if (bot && !botWorldSessionPtr->isLogingOut()) {
                itr = m_bots.erase(itr);                 // deletes bot player ptr inside this WorldSession PlayerBotMap
                botWorldSessionPtr->LogoutPlayer(true); // this will delete the bot Player object and PlayerbotAI object
                delete botWorldSessionPtr;              // finally delete the bot's WorldSession
            }
            else
                itr++;
            
        }
    }

}


void LuaBotManager::Update(uint32 diff) {

    for (auto bot : m_bots) {
        if (LuaBotAI* botAI = bot.second->GetLuaAI()) {
            botAI->Update(diff);
        }
    }

}



