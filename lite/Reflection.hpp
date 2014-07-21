#pragma once

#include "FieldInfo.hpp"
#include "MethodInfo.hpp"
#include "ReflectionPlugin.hpp"
#include "TypeInfo.hpp"

namespace lite
{
  class Reflection : public Singleton<Reflection>
  {
  private: // data

    vector<const TypeInfo*> types;

  public: // properties

    const vector<const TypeInfo*>& Types() const { return types; }

  public: // methods

    Reflection()
    {
      // Register void.
      RegisterType<void>().name = "void";

      // Register fundamental types.
      RegisterFundamentalType<bool>();
      RegisterFundamentalType<char>();
      RegisterFundamentalType<double>();
      RegisterFundamentalType<float>();
      RegisterFundamentalType<int>();
      RegisterFundamentalType<long>();
      RegisterFundamentalType<long long>();
      RegisterFundamentalType<short>();
      RegisterFundamentalType<signed char>();
      RegisterFundamentalType<unsigned char>();
      RegisterFundamentalType<unsigned short>();
      RegisterFundamentalType<unsigned int>();
      RegisterFundamentalType<unsigned long>();
      RegisterFundamentalType<unsigned long long>();
    }

    template <class T>
    void BindType()
    {
      static_assert(is_object<T>::value, "Only object types can be bound to Reflection.");

      // Bind the type.
      RegisterType<T>();
      Binding<T>{};
    }

    friend ostream& operator<<(ostream& os, const Reflection& r)
    {
      os << "Reflection ";
      for (auto& type : r.types)
      {
        os << "\n" << Tabs(1) << type->Name;
        os << "\n" << Tabs(2) << "Fields ";
        for (auto& field : type->Fields)
          os << "\n" << Tabs(3) << field;
        os << "\n" << Tabs(2) << "Methods ";
        for (auto& method: type->Methods)
          os << "\n" << Tabs(3) << method;
      }

      return os;
    }

  private: // methods

    template <class T>
    TypeInfo& RegisterFundamentalType()
    {
      // Register the type and set its name.
      TypeInfo& type = RegisterType<T>();
      type.name = typeid(T).name();

      // Add the default and copy constructor.
      type.methods.emplace_back(type.name, ConstructorFunction<T>);
      type.methods.emplace_back(type.name, ConstructorFunction<T, const T&>);

      return type;
    }

    template <class T>
    TypeInfo& RegisterType()
    {
      TypeInfo& type = detail::TypeOf<T>();
      types.push_back(&type);
      return type;
    }
  };

  // Base class for all binding structs.
  template <class Type>
  class BindingBase
  {
  public: // types

    typedef Type T;

    // Binds the type to reflection on construction.
    struct AutoBind
    {
      AutoBind()
      {
        Reflection::Instance().BindType<T>();
      }
    };

  public: // data

    // Global object to construct and 
    //  automatically bind the type.
    static AutoBind autoBind;

  public: // methods

    // Force a reference to the global object.
    ~BindingBase()
    {
      &autoBind;
    }

    // Binds all members to the type.
    template <class... Args>
    static void Bind(Args&&... args)
    {
      detail::TypeOf<T>().Bind<T>(forward<Args>(args)...);
    }

    template <class... Args>
    static Variant Constructor(Args... args)
    {
      return lite::ConstructorFunction<T, Args...>(forward<Args>(args)...);
    }
  };
  template <class Type> typename BindingBase<Type>::AutoBind BindingBase<Type>::autoBind;

  // All bindings to reflection specialize this struct to add in describe all members
  //  of the object.
  template <class Type>
  struct Binding :
    BindingBase < Type >
  {
    Binding(TypeInfo& type) : BindingBase(typeid(T).name())
    {
      type.Bind<T>(typeid(T).name());
    }
  };

  // Generic binding for all unique_ptr types.
  template <class T>
  struct unique_ptrBinding : BindingBase<unique_ptr<T>>
  {
    unique_ptrBinding()
    {
      Bind(
        "get", &T::get);
    }
  };

  // Generic binding for all vector types.
  template <class T>
  struct vectorBinding : BindingBase<vector<T>>
  {
    vectorBinding()
    {
      Bind(
        "at", Const(&T::at));
    }
  };

  // Bind std::string.
  template<> struct Binding<string> : BindingBase<string>
  {
    Binding()
    {
      Bind(
        Constructor<>);
    }
  };
} // namespace lite

namespace lite
{
  inline ostream& operator<<(ostream& os, const FieldInfo& f)
  {
    return os << f.type->Name << " " << f.Name;
  }

  inline ostream& operator<<(ostream& os, const MethodInfo& mi)
  {
    os << mi.returnType->Name << " " << mi.Name << "(";
    if (mi.ArgumentTypes.size())
    {
      os << mi.ArgumentTypes[0]->Name;
    }
    for (size_t i = 1; i < mi.ArgumentTypes.size(); ++i)
    {
      os << ", " << mi.ArgumentTypes[i]->Name;
    }
    return os << ")";
  }
} // namespace lite