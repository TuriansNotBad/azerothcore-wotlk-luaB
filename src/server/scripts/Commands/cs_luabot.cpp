/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Chat.h"
#include "Language.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "LuaBotManager.h"
#include <fstream>

using namespace Acore::ChatCommands;

class luabot_commandscript : public CommandScript
{
public:
    luabot_commandscript() : CommandScript("luabot_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable luabCmdTbl =
        {
            { "add",       HandleAddLuabCommand,      SEC_GAMEMASTER, Console::No },
            { "groupall",  HandleGroupallLuabCommand, SEC_GAMEMASTER, Console::No },
            { "remove",    HandleRemoveLuabCommand,   SEC_GAMEMASTER, Console::No },
            { "removeall", HandleRemoveAllLuabCommand,SEC_GAMEMASTER, Console::No },
            { "reset",     HandleResetLuabCommand,    SEC_GAMEMASTER, Console::No },
        };

        static ChatCommandTable commandTable =
        {
            { "luab", luabCmdTbl }
        };
        return commandTable;
    }

    static bool HandleAddLuabCommand(ChatHandler* handler, int logicID, std::string name)
    {
        // player not found
        Player* chr = handler->GetSession()->GetPlayer();
        if (!chr) {
            handler->SendSysMessage("Could not find master.");
            handler->SetSentErrorMessage(true);
            return false;
        }
        // can't find account
        uint32 accountId = sCharacterCache->GetCharacterAccountIdByGuid(chr->GetGUID());
        if (!accountId) {
            handler->SendSysMessage("Could not find master account ID.");
            handler->SetSentErrorMessage(true);
            return false;
        }
        // logic ID checks
        if (logicID < 0) {
            handler->SendSysMessage("All negative logic IDs are reserved for internal use.");
            handler->SetSentErrorMessage(true);
            return false;
        }
        // logic ID checks
        if (name.empty()) {
            handler->SendSysMessage("Syntax error. Character name required.");
            handler->SetSentErrorMessage(true);
            return false;
        }
        sLuaBotMgr.AddBot(name, accountId, logicID);
        return true;
    }

    static bool HandleGroupallLuabCommand(ChatHandler* handler)
    {
        Player* chr = handler->GetSession()->GetPlayer();
        sLuaBotMgr.GroupAll(chr);
        return true;
    }

    static bool HandleRemoveLuabCommand(ChatHandler* handler)
    {
        Player* bot = handler->getSelectedPlayer();
        if (!bot) {
            handler->SendSysMessage("Select a player first.");
            handler->SetSentErrorMessage(true);
            return false;
        }
        sLuaBotMgr.LogoutPlayerBot(bot->GetGUID());
        return true;
    }

    static bool HandleRemoveAllLuabCommand(ChatHandler* handler)
    {
        sLuaBotMgr.LogoutAllBots();
        return true;
    }

    static bool HandleResetLuabCommand(ChatHandler* handler)
    {
        sLuaBotMgr.LuaReload();
        return true;
    }

};

void AddSC_luabot_commandscript()
{
    new luabot_commandscript();
}
