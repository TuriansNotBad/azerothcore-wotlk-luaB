#ifndef MANGOS_LuaLibAI_H
#define MANGOS_LuaLibAI_H

#include "lua.hpp"
class LuaBotAI;

namespace LuaBindsAI {

	// Creates metatable for the AI userdata with name specified by AI::AI_MTNAME
	void BindAI(lua_State* L);
	// Creates metatable for the AI userdata with name specified by AI::AI_MTNAME.
	// Registers all the functions listed in LuaBindsBot::AI_BindLib with that metatable.
	void AI_CreateMetatable(lua_State* L);
	LuaBotAI** AI_GetAIObject(lua_State* L);
	int AI_AddTopGoal(lua_State* L);
	int AI_HasTopGoal(lua_State* L);

	int AI_Print(lua_State* L);
	int AI_GetPlayer(lua_State* L);


	static const struct luaL_Reg AI_BindLib[]{
		{"AddTopGoal", AI_AddTopGoal},
		{"HasTopGoal", AI_HasTopGoal},

		{"Print", AI_Print},
		{"GetPlayer", AI_GetPlayer},


		{NULL, NULL}
	};

}

#endif
