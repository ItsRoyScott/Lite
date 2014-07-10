#pragma once

#include "FieldInfo.hpp"
#include "MethodInfo.hpp"

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

  public: // methods

    TypeInfo() = default;
    TypeInfo(const TypeInfo&) = delete;
    ~TypeInfo() = default;
    TypeInfo& operator=(const TypeInfo&) = delete;

    // Add base-case; does nothing.
    void Add()
    {
    }

    // Adds a field given its name and pointer.
    template <class PointerT, class... Args>
    typename enable_if<IsField<PointerT>::value>::type
      Add(string name, PointerT ptr, Args&&... args)
    {
      fields.emplace_back(move(name), ptr);
      Add(forward<Args>(args)...);
    }

    // Adds a function given its name and pointer.
    template <class PointerT, class... Args>
    typename enable_if<FunctionTraits<PointerT>::IsFunction>::type
      Add(string name, PointerT ptr, Args&&... args)
    {
      methods.emplace_back(move(name), ptr);
      Add(forward<Args>(args)...);
    }

    // Initializes this type as a reference-type.
    template <class T>
    void InitializeReferenceType()
    {
      isReference = true;
      valueType = &TypeOf<T>();
    }

    // Calls Add functions recursively to initialize the type.
    template <class... Args>
    void operator()(string typeName, Args&&... args)
    {
      name = move(typeName);
      Add(forward<Args>(args)...);
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