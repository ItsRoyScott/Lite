#pragma once

#include "Essentials.hpp"
#include "ReflectionPlugin.hpp"

namespace lite // forward declarations
{
  template <class T> struct Binding;
  class MethodInfo;
  class NamespaceInfo;
  class TypeInfo;
  template <class... Args> struct TypeOfN;

  template <class T> const TypeInfo& TypeOf();
} // namespace lite

namespace lite // functions
{
  namespace detail
  {
    // Converts a std::function from one call-type to another.
    //  This allows us to do some shady stuff, like converting a
    //  function<void(T&, float)> to simply function<void()>.
    template <class CallT, class ResultCallT>
    function<ResultCallT> CastFunction(function<CallT> fn)
    {
      return reinterpret_cast<function<ResultCallT>&&>(fn);
    }
  } // namespace detail

  // Returns the global instance of the reflection plugin.
  inline ReflectionPlugin& GetReflectionPlugin()
  {
    static ReflectionPlugin plugin;
    return plugin;
  }

  // Returns the global instance of an type builder templatized to the object type.
  template <class T>
  inline ReflectionPlugin::TypeBuilder<T>& GetTypeBuilder()
  {
    static ReflectionPlugin::TypeBuilder<T> class_(GetReflectionPlugin());
    return class_;
  }
} // namespace lite

namespace lite // types
{
  // Creates a vector of TypeInfo pointers given a sequence of types.
  //  (Recursive case)
  template <class T, class... Args>
  struct TypeOfN < T, Args... >
  {
    static vector<const TypeInfo*> Get(vector<const TypeInfo*> types = {})
    {
      types.push_back(&TypeOf<T>());
      return TypeOfN<Args...>::Get(move(types));
    }
  };

  // Creates a vector of TypeInfo pointers given a sequence of types.
  //  (Base case)
  template <>
  struct TypeOfN < >
  {
    static vector<const TypeInfo*> Get(vector<const TypeInfo*> types = {})
    {
      return move(types);
    }
  };

  // Stores nifty information on a function type.
  template <class CallT>
  struct FunctionTraits
  {
    static const bool IsFunction = false;
  };

  // Stores nifty information on a function type:
  //  for the call-type, e.g. int(float, void*).
  template <class R, class... Args>
  struct FunctionTraits < R(Args...) >
  {
    static const size_t     ArgumentCount = tuple_size<tuple<Args...>>::value;
    typedef tuple<Args...>  ArgumentTuple;
    typedef              R(*CFunctionType)(Args...);
    typedef void            ClassType;
    static const bool       IsConstMemberFunction = false;
    static const bool       IsFunction = true;
    static const bool       IsMemberFunction = false;
    typedef              R(*Pointer)(Args...);
    typedef R               ReturnType;

    // Returns the typeless version of the passed in function.
    static function<void()> GenerateTypelessFunction(R(*fn)(Args...))
    {
      // Reinterpret cast the typed function to a typeless function.
      return detail::CastFunction<R(Args...), void()>(fn);
    }

    // Returns an array of reflected argument types.
    static vector<const TypeInfo*> RuntimeArguments()
    {
      return TypeOfN<Args...>::Get();
    }

    // Returns the reflected return type.
    static const TypeInfo& RuntimeReturnType()
    {
      return TypeOf<R>();
    }
  };

  // Stores nifty information on a function type:
  //  for the C function type, e.g. int(*)(float, void*).
  template <class R, class... Args>
  struct FunctionTraits < R(*)(Args...) > :
    FunctionTraits < R(Args...) >
  {};

  // Stores nifty information on a function type:
  //  for std::function.
  template <class R, class... Args>
  struct FunctionTraits < function<R(Args...)> > :
    FunctionTraits < R(Args...) >
  {};

  // Stores nifty information on a function type:
  //  for the non-const member function, e.g. int(Foo::*)(float).
  template <class R, class T, class... Args>
  struct FunctionTraits < R(T::*)(Args...) > :
    FunctionTraits < R(T&, Args...) >
  {
    typedef T         ClassType;
    static const bool IsMemberFunction = true;
    typedef     R(T::*Pointer)(Args...);

    // Returns the typeless version of the passed in function.
    static function<void()> GenerateTypelessFunction(R(T::*fn)(Args...))
    {
      // Capture the member function into a lambda.
      auto lambda = [=](T& self, Args... args) -> R
      {
        return (self.*fn)(forward<Args>(args)...);
      };

      // Reinterpret cast the typed function to a typeless function.
      return detail::CastFunction<R(T&, Args...), void()>(lambda);
    }
  };

  // Stores nifty information on a function type:
  //  for the const member function, e.g. int(Foo::*)(float).
  template <class R, class T, class... Args>
  struct FunctionTraits <R(T::*)(Args...) const> :
    FunctionTraits < R(const T&, Args...) >
  {
    typedef T         ClassType;
    static const bool IsConstMemberFunction = true;
    static const bool IsMemberFunction = true;
    typedef     R(T::*Pointer)(Args...) const;

    // Returns the typeless version of the passed in function.
    static function<void()> GenerateTypelessFunction(R(T::*fn)(Args...) const)
    {
      // Capture the member function into a lambda.
      auto lambda = [=](const T& self, Args... args) -> R
      {
        return (self.*fn)(forward<Args>(args)...);
      };

      // Reinterpret cast the typed function to a typeless function.
      return detail::CastFunction<R(const T&, Args...), void()>(lambda);
    }
  };

  // Whether the given type is a pointer to a data 
  //  member or a global object. (member case)
  template <class T>
  struct IsField :
    is_member_object_pointer<T>
  {};

  // Whether the given type is a pointer to a data 
  //  member or a global object. (global case)
  template <class FieldT>
  struct IsField<FieldT*> :
    is_object<FieldT>
  {};

} // namespace lite