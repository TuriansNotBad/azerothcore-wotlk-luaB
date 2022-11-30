
#include "LuaUtils.h"
#include "LuaLibAI.h"
#include "LuaBotAI.h"
#include "LuaLibUnit.h"


void LuaBindsAI::BindAI( lua_State* L ) {
	AI_CreateMetatable( L );
}


LuaBotAI** LuaBindsAI::AI_GetAIObject( lua_State* L ) {
	return (LuaBotAI**) luaL_checkudata( L, 1, LuaBotAI::MTNAME );
}


void LuaBindsAI::AI_CreateMetatable( lua_State* L ) {
	luaL_newmetatable( L, LuaBotAI::MTNAME );
	lua_pushvalue( L, -1 ); // copy mt cos setfield pops
	lua_setfield( L, -1, "__index" ); // mt.__index = mt
	luaL_setfuncs( L, AI_BindLib, 0 ); // copy funcs
	lua_pop( L, 1 ); // pop mt
}


int LuaBindsAI::AI_AddTopGoal(lua_State* L) {

	int nArgs = lua_gettop(L);

	if (nArgs < 3) {
		luaL_error(L, "AI.AddTopGoal - invalid number of arguments. 3 min, %d given", nArgs);
		//return 0;
	}

	LuaBotAI** ai = AI_GetAIObject(L);
	int goalId = luaL_checknumber(L, 2);

	Goal* topGoal = (*ai)->GetTopGoal();

	if (goalId != topGoal->GetGoalId() || topGoal->GetTerminated()) {
		double life = luaL_checknumber(L, 3);

		// (*goal)->Print();

		std::vector<GoalParamP> params;
		Goal_GrabParams(L, nArgs, params);
		// printf( "%d\n", params.size() );

		Goal** goalUserdata = Goal_CreateGoalUD(L, (*ai)->AddTopGoal(goalId, life, params, L)); // ud on top of the stack
		// duplicate userdata for return result
		lua_pushvalue(L, -1);
		// save userdata
		(*goalUserdata)->SetRef(luaL_ref(L, LUA_REGISTRYINDEX)); // pops the object as well
		(*goalUserdata)->CreateUsertable();
	}
	else {
		// leave topgoal's userdata as return result for lua
		lua_rawgeti(L, LUA_REGISTRYINDEX, topGoal->GetRef());
	}

	return 1;

}


int LuaBindsAI::AI_HasTopGoal(lua_State* L) {
	LuaBotAI* ai = *AI_GetAIObject(L);
	int id = luaL_checkinteger(L, 2);
	if (ai->GetTopGoal()->GetTerminated())
		lua_pushboolean(L, false);
	else
		lua_pushboolean(L, ai->GetTopGoal()->GetGoalId() == id);
	return 1;
}



int LuaBindsAI::AI_GetPlayer(lua_State* L) {
	LuaBotAI* ai = *AI_GetAIObject(L);
	ai->PushPlayerUD(L);
	return 1;
}


int LuaBindsAI::AI_Print( lua_State* L ) {
	LuaBotAI** ai = AI_GetAIObject( L );
	( *ai )->Print();
	return 0;
}
