#pragma once

#include "ReflectionPlugin.hpp"
#include "ReflectionUtility.hpp"

namespace lite
{
  // Meta information on a method in C++.
  class MethodInfo
  {
  private: // data

    vector<const TypeInfo*> argumentTypes;
    function<void()>        func;
    string                  name;
    const TypeInfo*         returnType;

  public: // properties

    // Array of TypeInfo objects describing the argument types.
    const vector<const TypeInfo*>& ArgumentTypes() const { return argumentTypes; }

    // Name of the method.
    const string& Name() const { return name; }

    // Return type as a TypeInfo.
    const TypeInfo* ReturnType() const { return returnType; }

  public: // methods

    // Constructs given the name and function pointer.
    template <class FuncPtr>
    MethodInfo(string name_, FuncPtr func_, bool isField = false)
    {
      typedef FunctionTraits<FuncPtr> Traits;

      argumentTypes = Traits::RuntimeArguments();
      func = Traits::GenerateTypelessFunction(func_);
      name = move(name_);
      returnType = &TypeOf<Traits::ReturnType>();

      ReflectionPlugin::OnNewMethod(name, func_);
    }

    // Retrieves the typed function object.
    template <class CallT>
    function<CallT> AsFunction() const
    {
      typedef FunctionTraits<CallT> Traits;

      // Match the return types.
      if (&Traits::RuntimeReturnType() != returnType) return{};

      // Match the number of arguments.
      vector<const TypeInfo*> givenArguments = Traits::RuntimeArguments();
      if (givenArguments.size() != argumentTypes.size()) return{};

      // Match each argument.
      for (size_t i = 0; i < argumentTypes.size(); ++i)
      {
        if (givenArguments[i] != argumentTypes[i]) return{};
      }

      // Reinterpret cast to the requested function type.
      return TypelessToTypedFunction<CallT>(func);
    }
  };
} // namespace lite