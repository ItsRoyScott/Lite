#pragma once

#include "Essentials.hpp"
#include "FileTime.hpp"
#include "LuaBridgeInclude.hpp"
#include "BasicIO.hpp"

#define LUA(call) SCOPE(int top = lua_gettop(L); call; PrintLuaErrors(L, top))

namespace lite
{
  inline void PrintLuaErrors(lua_State* L, int resultingTop)
  {
    int newTop = lua_gettop(L);

    // Display strings as warnings in the console.
    for (int i = resultingTop ? resultingTop : 1; i <= newTop; ++i)
    {
      if (lua_isstring(L, i))
      {
        Warn(lua_tostring(L, i));
      }
    }
    lua_pop(L, newTop - resultingTop);
  }

  class LuaScript
  {
  private: // data

    string bytecodePath;
    FileTime bytecodeTime;
    lua_State* L = nullptr;
    string name;
    string stringPath;
    FileTime stringTime;

  public: // methods

    LuaScript(string name_, lua_State& luaState_) :
      L(&luaState_),
      name(move(name_))
    {
      Reload();
    }

    bool Reload()
    {
      string baseName = config::Scripts + name;
      bytecodePath = baseName + ".luac";
      stringPath = baseName + ".lua";

      vector<unsigned char> bytecode;

      // Check whether the bytecode and/or string exists.
      bool readBytecode = GetPathType(bytecodePath) == PathType::File;
      bool readString = GetPathType(stringPath) == PathType::File;

      // Read the script and convert to bytecode.
      if (readBytecode && readString)
      {
        // Choose the newer file to read.
        if (FileTime(bytecodePath).CompareTo(FileTime(stringPath)) == FileTime::Later)
        {
          bytecode = ReadString();
        }
        else
        {
          bytecode = ReadBytecode();
        }
      }
      else if (readBytecode)
      {
        bytecode = ReadBytecode();
      }
      else if (readString)
      {
        bytecode = ReadString();
      }

      WarnIf(bytecode.empty(), "Load failed for script: " + name);

      if (!bytecode.empty())
      {
        // Load the buffer into the Lua state.
        LUA(luaL_loadbuffer(L, (char*) bytecode.data(), bytecode.size(), name.c_str()));
      }
    }

    bool ShouldReload() const
    {
      if (stringTime && stringTime.CompareTo(FileTime(stringPath)) == FileTime::Later)
      {
        return true;
      }
      if (bytecodeTime && bytecodeTime.CompareTo(FileTime(bytecodePath)) == FileTime::Later)
      {
        return true;
      }
      return false;
    }

  private: // methods

    static int LuaWriter(lua_State*, const void* buffer, size_t size, ofstream* file)
    {
      file->write(reinterpret_cast<const char*>(buffer), size);
      return 0;
    }

    vector<unsigned char> ReadBytecode()
    {
      vector<unsigned char> data;
      if (ReadEntireFile(bytecodePath, data))
      {
        bytecodeTime = FileTime(bytecodePath);
      }
      return move(data);
    }

    vector<unsigned char> ReadString()
    {
      vector<unsigned char> data;
      string fileString;

      if (ReadEntireFile(stringPath, fileString))
      {
        stringTime = FileTime(stringPath);

        // Load the file into a temporary Lua state.
        auto tempState = unique_ptr<lua_State, void(*)(lua_State*)>(luaL_newstate(), &lua_close);
        luaL_loadfile(tempState.get(), stringPath.c_str());

        // Dump the bytecode to file.
        auto outputFile = ofstream(bytecodePath, ofstream::binary);
        lua_dump(tempState.get(), (lua_Writer) LuaWriter, &outputFile);
        outputFile.close();

        // Read the resulting bytecode.
        data = ReadBytecode();
      }

      return move(data);
    }
  };

  class Scripting : public Singleton<Scripting>
  {
  private: // data

    // Manages Lua's current embedded state.
    unique_ptr<lua_State, void(*)(lua_State*)> luaState;

  public: // methods

    // Initializes Lua.
    Scripting() :
      luaState(luaL_newstate(), &lua_close)
    {
      luaL_openlibs(luaState.get());
    }

    void Clear()
    {
      luaState.reset();
    }

    // Returns the global namespace for binding C++ types to Lua.
    luabridge::Namespace GetGlobalNamespace()
    {
      return luabridge::getGlobalNamespace(luaState.get());
    }

    lua_State* GetLuaState() const
    {
      return luaState.get();
    }
  };

  template <class T>
  unique_ptr<Namespace::Class<T>>& getClass()
  {
    static unique_ptr<Namespace::Class<T>> cls;
    return cls;
  }

  template <class T>
  luabridge::Namespace::Class <T>& beginClass(char const* name)
  {
    unique_ptr<Namespace::Class<T>>& cls = getClass<T>();
    if (!cls)
    {
      cls = make_unique<Namespace::Class<T>>(Scripting::Instance().GetGlobalNamespace().beginClass<T>(name));
    }
    return *cls;
  }

  template <class T>
  void endClass()
  {
    unique_ptr<Namespace::Class<T>>& cls = getClass<T>();
    cls->endClass();
    cls.reset();
  }
} // namespace lite
