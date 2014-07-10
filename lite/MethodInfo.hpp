#pragma once

#include "ReflectionPlugin.hpp"
#include "ReflectionUtility.hpp"

namespace lite
{
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

    template <class FuncPtr>
    MethodInfo(string name_, FuncPtr func_, bool isField = false)
    {
      typedef FunctionTraits<FuncPtr> Traits;

      argumentTypes = Traits::RuntimeArguments();
      func = Traits::GenerateTypelessFunction(func_);
      name = move(name_);
      returnType = &TypeOf<Traits::ReturnType>();

      if (!isField)
      {
        ReflectionPlugin::OnNewMethod(name, func_);
      }
    }

    template <class CallT>
    function<CallT> Get() const
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

  // A field is just a special type of method.
  class FieldInfo : public MethodInfo
  {
  public: // methods

    template <class FieldT>
    FieldInfo(string name_, FieldT* fieldPtr) :
      MethodInfo(move(name_), [=] { return fieldPtr; }, false)
    {
    }

    template <class FieldT, class T>
    FieldInfo(string name_, FieldT T::*fieldPtr) :
      MethodInfo(move(name_), [=](T& self) -> FieldT { return (self.*fieldPtr); }, false)
    {
    }
  };
} // namespace lite