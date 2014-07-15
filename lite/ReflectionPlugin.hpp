#pragma once

#include "LuaCppInterfaceInclude.hpp"
#include "Scripting.hpp"
#include "Variant.hpp"

namespace lite
{
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
        typeTable(Scripting::Instance().Lua.CreateTable())
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
        Scripting::Instance().Lua.GetGlobalEnvironment().Set(typeName, typeTable);
      }

      // Called when a non-object type (e.g. int, void*) bind is ending:
      //  Does nothing for now.
      void EndValueType(const string&)
      {}

      void NewConstructor(const string&, Variant(*)())
      {
        // Create a Lua function which wraps the constructor.
        LuaFunction<LuaUserdata<T>()> luaConstructor = 
          Scripting::Instance().Lua.CreateFunction<LuaUserdata<T>()>(ConstructorFunction);

        // Set the constructor to Lua's new function for this type.
        typeTable.Set("new", luaConstructor);
      }

      template <class Arg1, class... Args>
      void NewConstructor(const string&, Variant(*)(Arg1, Args...))
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
        bindFunctions.push_back([=](LuaUserdata<T>& userdata)
        {
          userdata.Bind(methodName, funcPtr);
        });
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
        LuaUserdata<T> instance = Scripting::Instance().Lua.CreateUserdata<T>(new T);

        // Bind all members to the LuaUserdata.
        for (auto& bindFunction : CurrentInstance()->bindFunctions)
        {
          bindFunction(instance);
        }

        return move(instance);
      }
    };
  };

} // namespace lite