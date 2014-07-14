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
      // Bind  void.
      TypeInfo& voidType = detail::TypeOf<void>();
      voidType.Bind<void>("void");
      types.push_back(&voidType);
    }

    template <class T>
    void BindType()
    {
      static_assert(is_object<T>::value, "Only object types can be bound to Reflection.");

      // Bind the value type.
      TypeInfo& valueType = detail::TypeOf<T>();
      Bind < T > { valueType };
      types.push_back(&valueType);

      // Bind the reference type.
      TypeInfo& referenceType = detail::TypeOf<T&>();
      referenceType.InitializeReferenceType<T>();
      types.push_back(&referenceType);
    }
  };

  // Base class for all binding structs.
  template <class Type>
  class BindBase
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

    // Calls the ReflectionPlugin's OnTypeBegin.
    BindBase(const string& name)
    {
      NotifyPluginOnBeginType<T>(name);
    }

    // Force a reference to the global object.
    ~BindBase()
    {
      &autoBind;
      NotifyPluginOnEndType<T>(TypeOf<T>().Name);
    }

  private: // methods

    // Notifies the plugin that we are starting to bind a class type.
    template <class T>
    typename enable_if<is_class<T>::value>::type
      NotifyPluginOnBeginType(const string& name)
    {
      GetReflectionPluginObjectBuilder<T>().BeginClassType(name);
    }

    // Notifies the plugin that we are starting to bind a value type.
    template <class T>
    typename enable_if<!is_class<T>::value>::type
      NotifyPluginOnBeginType(const string& name)
    {
      GetReflectionPluginObjectBuilder<T>().BeginValueType(name);
    }

    // Notifies the plugin that we are finishing a class type.
    template <class T>
    typename enable_if<is_class<T>::value>::type
      NotifyPluginOnEndType(const string& name)
    {
      GetReflectionPluginObjectBuilder<T>().EndClassType(name);
    }

    // Notifies the plugin that we are finishing a value type.
    template <class T>
    typename enable_if<!is_class<T>::value>::type
      NotifyPluginOnEndType(const string& name)
    {
      GetReflectionPluginObjectBuilder<T>().EndValueType(name);
    }
  };
  template <class Type> typename BindBase<Type>::AutoBind BindBase<Type>::autoBind;

  // Generic binding. All other bindings
  //  specialize this struct.
  template <class Type>
  struct Bind :
    BindBase < Type >
  {
    Bind(TypeInfo& type) : BindBase(typeid(T).name())
    {
      type.Bind<T>(typeid(T).name());
    }
  };
} // namespace lite

// Macro to make the binding process easier.
#define reflect(class_, ...) \
  template <> \
  struct Bind <class_> : BindBase<class_> \
  { \
    Bind(TypeInfo& type) : BindBase(#class_) \
    { \
      type.Bind<T> \
      ( \
        #class_, \
        __VA_ARGS__  \
      ); \
    } \
  }

namespace lite
{
  // Reflect fundamental types.
  //reflect(bool);
  //reflect(char);
  //reflect(double);
  //reflect(float);
  //reflect(int);
  //reflect(long);
  //reflect(long long);
  //reflect(nullptr_t);
  //reflect(short);
  //reflect(signed char);
  //reflect(unsigned char);
  //reflect(unsigned short);
  //reflect(unsigned int);
  //reflect(unsigned long);
  //reflect(unsigned long long);
  //reflect(wchar_t);
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