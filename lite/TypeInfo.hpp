#pragma once

#include "MethodInfo.hpp"

namespace lite
{
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
    const vector<FieldInfo>& Fields() const { return fields; }

    // Whether this type represents a reference to a value-type.
    bool IsReference() const { return isReference; }

    // Array of this type's methods.
    const vector<MethodInfo>& Methods() const { return methods; }

    // Name of the type.
    const string& Name() const { return name; }

    // Returns the value-type if this type is a reference.
    const TypeInfo* ValueType() const { return valueType; }

  public: // methods

    TypeInfo() = default;

    void Add()
    {
    }

    template <class PointerT, class... Args>
    typename enable_if<IsField<PointerT>::value>::type
      Add(string name, PointerT ptr, Args&&... args)
    {
      fields.emplace_back(move(name), ptr);
      Add(forward<Args>(args)...);
    }

    template <class PointerT, class... Args>
    typename enable_if<FunctionTraits<PointerT>::IsFunction>::type
      Add(string name, PointerT ptr, Args&&... args)
    {
      methods.emplace_back(move(name), ptr);
      Add(forward<Args>(args)...);
    }

    template <class T>
    void InitializeReferenceType()
    {
      isReference = true;
      valueType = &TypeOf<T>();
    }

    template <class... Args>
    void operator()(string typeName, Args&&... args)
    {
      name = move(typeName);
      Add(forward<Args>(args)...);
    }
  };

  namespace detail
  {
    template <class T>
    TypeInfo& TypeOf()
    {
      static TypeInfo type;
      return type;
    }
  } // namespace detail

  template <class T>
  const TypeInfo& TypeOf()
  {
    return detail::TypeOf<typename remove_cv<T>::type>();
  }
} // namespace lite