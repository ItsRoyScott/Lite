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
    type_index              cFunctionType = typeid(void);
    function<void()>        func;
    string                  name;
    const TypeInfo*         returnType;

  public: // properties

    // Array of TypeInfo objects describing the argument types.
    const vector<const TypeInfo*>& ArgumentTypes = argumentTypes;

    // Name of the method.
    const string& Name = name;

    // Return type as a TypeInfo.
    const TypeInfo* const& ReturnType = returnType;

  public: // methods

    MethodInfo() = delete;

    MethodInfo(MethodInfo&& b) :
      argumentTypes(move(b.argumentTypes)),
      cFunctionType(move(b.cFunctionType)),
      func(move(b.func)),
      name(move(b.name)),
      returnType(b.returnType)
    {}
    
    MethodInfo& operator=(MethodInfo&& b)
    {
      argumentTypes = move(b.argumentTypes);
      cFunctionType = move(b.cFunctionType);
      func = move(b.func);
      name = move(b.name);
      returnType = b.returnType;
    
      return *this;
    }

    MethodInfo(const MethodInfo&) = delete;
    MethodInfo& operator=(const MethodInfo&) = delete;

    // Constructs given the name and function pointer.
    template <class FuncPtr>
    MethodInfo(string name_, FuncPtr func_)
    {
      typedef FunctionTraits<FuncPtr> Traits;

      argumentTypes = Traits::RuntimeArguments();
      cFunctionType = typeid(Traits::CFunctionType);
      func = Traits::GenerateTypelessFunction(func_);
      name = move(name_);
      returnType = &TypeOf<Traits::ReturnType>();
    }

    ~MethodInfo() = default;

    // Retrieves the typed function object.
    template <class CallT>
    function<CallT> AsFunction() const
    {
      typedef FunctionTraits<CallT> Traits;

      // Verify the call type is correct.
      if (cFunctionType != typeid(Traits::CFunctionType)) return{};

      // Reinterpret cast to the requested function type.
      return detail::CastFunction<void(), CallT>(func);
    }

    // Formats the MethodInfo into an ostream.
    friend ostream& operator<<(ostream& os, const MethodInfo& mi);
  };

  template <class T, class... Args>
  Variant ConstructorFunction(Args... args)
  {
    return T(forward<Args>(args)...);
  }

  template <class... Args>
  struct Overloaded
  {
    template <class RetT, class ClassT, class FuncPtr = RetT(ClassT::*)(Args...)>
    static FuncPtr Get(RetT(ClassT::*fn)(Args...))
    {
      return fn;
    }

    template <class RetT, class FuncPtr = RetT(*)(Args...)>
    static FuncPtr Get(RetT(*fn)(Args...))
    {
      return fn;
    }
  };

} // namespace lite
