#pragma once

//#pragma warning(push, 1)
//#include "LuaCppInterface/luacppinterface.h"
//#pragma warning(pop)

#pragma comment(lib, "lua52/lua52.lib")

//#ifdef _DEBUG
//#pragma comment(lib, "LuaCppInterface/LuaCppInterface-d.lib")
//#else
//#pragma comment(lib, "LuaCppInterface/LuaCppInterface.lib")
//#endif

#include "lua52/lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include "LuaBridge/LuaBridgeExtras.h"
#include "LuaBridge/RefCountedObject.h"
#include "LuaBridge/RefCountedPtr.h"