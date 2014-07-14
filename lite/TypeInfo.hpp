#pragma once

#include "FieldInfo.hpp"
#include "MethodInfo.hpp"
#include "ReflectionPlugin.hpp"

namespace lite
{
  // Meta information regarding a type in C++.
  class TypeInfo
  {
  private: // data

    vector<FieldInfo>   fields;
    bool                isReference = false;
    vector<MethodInfo>  methods;
    string              name;
    const TypeInfo*     valueType = nullptr;

  public: // properties

    // Array of this type's fields.
    const vector<FieldInfo>& Fields = fields;

    // Whether this type represents a reference to a value-type.
    const bool& IsReference = isReference;

    // Array of this type's methods.
    const vector<MethodInfo>& Methods = methods;

    // Name of the type.
    const string& Name = name;

    // Returns the value-type if this type is a reference.
    const TypeInfo* const& ValueType = valueType;

  public: // const methods

    // Finds a method using a predicate function. The signature
    //  of the predicate is bool(const MethodInfo&).
    template <class Predicate>
    const MethodInfo* FindMethodBy(Predicate pred) const
    {
      for (const MethodInfo& method : methods)
      {
        if (pred(method)) return &method;
      }
      return nullptr;
    }

    // Finds a method by name. (May return null)
    const MethodInfo* GetMethod(const string& name) const
    {
      return FindMethodBy([&](const MethodInfo& m) { return m.Name == name; });
    }

  public: // non-const methods

    TypeInfo() = default;
    TypeInfo(const TypeInfo&) = delete;
    TypeInfo& operator=(const TypeInfo&) = delete;
    ~TypeInfo() = default;

    // Add base-case; does nothing.
    template <class T>
    void Add()
    {}

    // Adds a field given its name and pointer.
    template <class T, class ValueT, class... Args>
    void Add(string name, ValueT* ptr, Args&&... args)
    {
      NotifyPluginOnNewStaticField<T>(name, ptr);
      fields.emplace_back(move(name), ptr);
      Add<T>(forward<Args>(args)...);
    }

    template <class T, class ValueT, class... Args>
    void Add(string name, ValueT T::*fieldPtr, Args&&... args)
    {
      NotifyPluginOnNewField<T>(name, fieldPtr);
      fields.emplace_back(move(name), fieldPtr);
      Add<T>(forward<Args>(args)...);
    }

    // Adds a member function given its name and pointer.
    template <class T, class ClassT, class RetT, class... FuncArgs, class... Args>
     void Add(string name, RetT(ClassT::*ptr)(FuncArgs...), Args&&... args)
    {
      NotifyPluginOnNewMethod<T>(name, ptr);
      methods.emplace_back(move(name), ptr);
      Add<T>(forward<Args>(args)...);
    }

    // Adds a member function given its name and pointer.
    template <class T, class ClassT, class RetT, class... FuncArgs, class... Args>
    void Add(string name, RetT(ClassT::*ptr)(FuncArgs...) const, Args&&... args)
    {
      NotifyPluginOnNewMethod<T>(name, ptr);
      methods.emplace_back(move(name), ptr);
      Add<T>(forward<Args>(args)...);
    }

    template <class T, class FieldT1, class T1, class FieldT2, class T2, class... Args>
    void Add(string name, FieldT1(T1::*getter)() const, void(T2::*setter)(FieldT2), Args&&... args)
    {
      // Notify the plugin of the new field.
      NotifyPluginOnNewProperty<T>(name, getter, setter);

      // Create the field.
      fields.emplace_back(move(name), getter, setter);

      // Perfect-forward the rest of the arguments to Add.
      Add<T>(forward<Args>(args)...);
    }

    // Initializes this type as a reference-type.
    template <class T>
    void InitializeReferenceType()
    {
      isReference = true;
      valueType = &TypeOf<T>();
    }

    // Calls Add functions recursively to initialize the type.
    template <class T, class... Args>
    void Bind(string typeName, Args&&... args)
    {
      name = move(typeName);
      Add<T>(forward<Args>(args)...);
    }

    // Formats the type info into an ostream.
    friend ostream& operator<<(ostream& os, const TypeInfo& ti)
    {
      os << ti.Name;
    }

  private: // methods

    // Notifies the plugin when we have a new member field.
    template <class T, class ValueT>
    void NotifyPluginOnNewField(const string& name, ValueT T::*fieldPtr)
    {
      GetReflectionPluginObjectBuilder<T>().NewField(name, fieldPtr);
    }

    template <class T, class FuncPtr>
    void NotifyPluginOnNewMethod(const string& name, FuncPtr methodPtr)
    {
      GetReflectionPluginObjectBuilder<T>().NewMethod(name, methodPtr);
    }

    // Notifies the plugin when we create a field we have a new getter/setter pair.
    template <class T, class GetRetT, class GetClassT, class SetArgT, class SetClassT>
    void NotifyPluginOnNewProperty(
      const string& name,
      GetRetT(GetClassT::*getter)() const,
      void(SetClassT::*setter)(SetArgT))
    {
      GetReflectionPluginObjectBuilder<T>().NewProperty(name, getter, setter);
    }

    // Notifies the plugin when we have a new static field.
    template <class T, class ValueT>
    void NotifyPluginOnNewStaticField(const string& name, ValueT* fieldPointer)
    {
      GetReflectionPluginObjectBuilder<T>().NewStaticField(name, fieldPointer);
    }

    template <class T, class PointerT>
    void NotifyPluginOnNewStaticFunction(const string& name, PointerT func)
    {
      GetReflectionPluginObjectBuilder<T>().NewStaticFunction(name, func);
    }
  };

  namespace detail
  {
    // Stores the static data for a type info.
    template <class T>
    TypeInfo& TypeOf()
    {
      static TypeInfo type;
      return type;
    }
  } // namespace detail

  // Returns a TypeInfo given its C++ type.
  template <class T>
  const TypeInfo& TypeOf()
  {
    return detail::TypeOf<typename remove_cv<T>::type>();
  }
} // namespace lite