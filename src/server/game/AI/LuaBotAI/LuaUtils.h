#ifndef MANGOS_LuaUtils_H
#define MANGOS_LuaUtils_H

#include "lua.hpp"

bool luaL_checkboolean(lua_State* L, int idx);
void lua_pushplayerornil(lua_State* L, Player* u);
void lua_pushunitornil(lua_State* L, Unit* u);
void* luaL_checkudwithfield(lua_State* L, int idx, const char* fieldName);
namespace LuaBindsAI {
	bool IsValidHostileTarget(Unit* me, Unit const* pTarget);
    void SatisfyItemRequirements(Player* me, ItemTemplate const* pItem);
    void GetEquipSlotsForType(InventoryType type, uint32 SubClass, uint8 slots[4], uint8 classId, bool canDualWield);
    // Select a random element from a container. Note: make sure you explicitly empty check the container
    template <class C>
    typename C::value_type const& SelectRandomContainerElement(C const& container)
    {
        typename C::const_iterator it = container.begin();
        std::advance(it, urand(0, container.size() - 1));
        return *it;
    }
    bool IsShieldClass(uint8 playerClass);
    uint32 GetHighestKnownArmorProficiency(Player* me);
}


#endif
