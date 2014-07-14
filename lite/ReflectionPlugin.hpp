#pragma once

#include "ReflectionUtility.hpp"
#include "Scripting.hpp"

namespace lite
{
  // Hooks into the Reflection's binding process to automatically bind
  //  functions to Lua (using LuaBridge, for now).
  struct ReflectionPlugin
  {
    template <class T>
    struct ObjectBuilder
    {
      ObjectBuilder(ReflectionPlugin&) {}

      // Called when an object type bind is starting:
      //  Used to begin the class binding process through LuaBridge.
      void BeginClassType(const string& className)
      {
        beginClass<T>(className.c_str()).addConstructor();
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
        endClass<T>();
      }

      // Called when a non-object type (e.g. int, void*) bind is ending:
      //  Does nothing for now.
      void EndValueType(const string&)
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
        typedef FunctionTraits<FuncPtr> Traits;
        getClass<Traits::ClassType>()->addFunction(methodName.c_str(), funcPtr);
      }

      template <class GetterPtr, class SetterPtr>
      void NewProperty(const string&, GetterPtr, SetterPtr)
      {}
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