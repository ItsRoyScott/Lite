#pragma once

#include "ReflectionUtility.hpp"
#include "Scripting.hpp"

namespace lite
{
  // Hooks into the Reflection's binding process to automatically bind
  //  functions to Lua (using LuaBridge, for now).
  struct ReflectionPlugin
  {
    // Called when a new member function is bound to the type:
    //  This adds the function to the LuaBridge Class object.
    template <class FuncPtr>
    static typename enable_if<FunctionTraits<FuncPtr>::IsMemberFunction>::type
      OnNewMethod(const string& methodName, FuncPtr funcPtr)
    {
      typedef FunctionTraits<FuncPtr> Traits;
      getClass<Traits::ClassType>()->addFunction(methodName.c_str(), funcPtr);
    }

    // Called when a new static function is bound to the type:
    //  Does nothing for now.
    template <class FuncPtr>
    static typename enable_if<!FunctionTraits<FuncPtr>::IsMemberFunction>::type
      OnNewMethod(const string&, FuncPtr)
    {
    }

    // Called when an object type bind is starting:
    //  Used to begin the class binding process through LuaBridge.
    template <class T>
    static typename enable_if<is_class<T>::value>::type
      OnTypeBegin(const string& className)
    {
      beginClass<T>(className.c_str()).addConstructor();
    }

    // Called when a non-object type (e.g. int, void*) bind is starting:
    //  Does nothing for now.
    template <class T>
    static typename enable_if<!is_class<T>::value>::type
      OnTypeBegin(const string&)
    {
    }

    // Called when an object type bind is ending:
    //  Used to end the class binding process through LuaBridge.
    template <class T>
    static typename enable_if<is_class<T>::value>::type
      OnTypeEnd(const string&)
    {
      endClass<T>();
    }

    // Called when a non-object type (e.g. int, void*) bind is ending:
    //  Does nothing for now.
    template <class T>
    static typename enable_if<!is_class<T>::value>::type
      OnTypeEnd(const string&)
    {
    }
  };
} // namespace lite