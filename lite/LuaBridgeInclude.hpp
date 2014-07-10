#pragma once

extern "C"
{
  #include "lua52/lua.h"
  #include "lua52/lualib.h"
  #include "lua52/lauxlib.h"
}
#include "LuaBridge/LuaBridge.h"

#pragma comment(lib, "lua52/lua52.lib")

namespace lite
{
  using namespace luabridge;
} // namespace lite