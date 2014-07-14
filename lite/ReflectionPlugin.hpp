#pragma once

#include "LuaCppInterfaceInclude.hpp"
#include "ReflectionUtility.hpp"
#include "Scripting.hpp"

namespace lite
{
  static Lua& LuaInstance()
  {
    static Lua lua;
    return lua;
  }

  // Hooks into the Reflection's binding process to automatically bind
  //  functions to Lua (using LuaBridge, for now).
  class ReflectionPlugin : public LightSingleton<ReflectionPlugin>
  {
  public: // types

    template <class T>
    class ObjectBuilder : public LightSingleton<ObjectBuilder<T>>
    {
    private: // data

      vector<function<void(LuaUserdata<T>&)>> bindFunctions;
      string typeName;
      LuaTable typeTable;

    public: // methods

      ObjectBuilder(ReflectionPlugin&) :
        typeTable(LuaInstance().CreateTable())
      {}

      // Called when an object type bind is starting:
      //  Used to begin the class binding process through LuaBridge.
      void BeginClassType(const string& className)
      {
        typeName = className;
      }

      // Called when a non-object type (e.g. int, void*) bind is starting:
      //  Does nothing for now.
      void BeginValueType(const string&)
      {
      }

      // Called when an object type bind is ending:
      //  Used to end the class binding process through LuaBridge.
      void EndClassType(const string&)
      {
        // Make the type accessible.
        LuaInstance().GetGlobalEnvironment().Set(typeName, typeTable);
      }

      // Called when a non-object type (e.g. int, void*) bind is ending:
      //  Does nothing for now.
      void EndValueType(const string&)
      {}

      void NewConstructor(const string&, void(*)(void*))
      {
        // Create a Lua function which wraps the constructor.
        LuaFunction<LuaUserdata<T>()> luaConstructor = 
          LuaInstance().CreateFunction<LuaUserdata<T>()>(ConstructorFunction);

        // Set the constructor to Lua's new function for this type.
        typeTable.Set("new", luaConstructor);
      }

      template <class Arg1, class... Args>
      void NewConstructor(const string&, void(*)(void*, Arg1, Args...))
      {}

      template <class FieldPtr>
      void NewField(const string&, FieldPtr)
      {}

      // Called when a new static function is bound to the type:
      //  Does nothing for now.
      template <class FuncPtr>
      void NewFunction(const string&, FuncPtr)
      {}

      // Called when a new member function is bound to the type:
      //  This adds the function to the LuaBridge Class object.
      template <class FuncPtr>
      void NewMethod(const string& methodName, FuncPtr funcPtr)
      {
      }

      template <class GetterPtr, class SetterPtr>
      void NewProperty(const string&, GetterPtr, SetterPtr)
      {}

      template <class FuncPtr>
      void NewStaticFunction(const string&, FuncPtr)
      {}

    private: // methods

      static LuaUserdata<T> ConstructorFunction()
      {
        // Create the userdata instance.
        LuaUserdata<T> instance = LuaInstance().CreateUserdata<T>(new T);

        // Bind all members to the LuaUserdata.
        for (auto& bindFunction : ObjectBuilder<T>::CurrentInstance()->bindFunctions)
        {
          bindFunction(instance);
        }

        return move(instance);
      }
    };
  };

  // Returns the global instance of the reflection plugin.
  inline ReflectionPlugin& GetReflectionPlugin()
  {
    static ReflectionPlugin plugin;
    return plugin;
  }

  template <class T>
  inline ReflectionPlugin::ObjectBuilder<T>& GetReflectionPluginObjectBuilder()
  {
    static ReflectionPlugin::ObjectBuilder<T> class_(GetReflectionPlugin());
    return class_;
  }
} // namespace lite