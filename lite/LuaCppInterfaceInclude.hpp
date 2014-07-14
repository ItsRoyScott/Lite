#pragma once

#pragma warning(push, 1)
#include "LuaCppInterface/luacppinterface.h"
#pragma warning(pop)

#pragma comment(lib, "lua52/lua52.lib")

#ifdef _DEBUG
#pragma comment(lib, "LuaCppInterface/LuaCppInterface-d.lib")
#else
#pragma comment(lib, "LuaCppInterface/LuaCppInterface.lib")
#endif