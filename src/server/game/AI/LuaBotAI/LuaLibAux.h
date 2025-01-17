#ifndef MANGOS_LuaLibAux_H
#define MANGOS_LuaLibAux_H


#include "lua.hpp"


namespace LuaBindsAI {
	void BindVmangos(lua_State* L);
	int GetUnitByGuid(lua_State* L);
	int GetPlayerByGuid(lua_State* L);
	uint64 GetRawGuidFromString(lua_State* L, int n);
}

#endif
