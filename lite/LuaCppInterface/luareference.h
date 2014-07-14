#ifndef LUAREFERENCE_H
#define LUAREFERENCE_H

#include <memory>
#include "luatype.h"
#include "lua52/lua.hpp"

class LuaTable;
class LuaReference
{
	int ref;

protected:
	std::shared_ptr<lua_State> state;

public:
	LuaReference(std::shared_ptr<lua_State> state, int index);
	LuaReference(const LuaReference& other);
	LuaReference& operator=(const LuaReference& other);
	virtual ~LuaReference();

	// push the object to the stack you are working on
	void PushToStack(lua_State* currentState) const;
	LuaType::Value GetType() const;
	std::shared_ptr<lua_State> GetState();
	LuaTable GetMetaTable() const;
};

#endif // LUAREFERENCE_H
