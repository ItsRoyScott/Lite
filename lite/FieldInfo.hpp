#pragma once

#include "Essentials.hpp"
#include "ReflectionUtility.hpp"

namespace lite
{
  // Meta information on a field in C++.
  class FieldInfo
  {
  private: // data

    function<void*(void*)>    getFieldPointer;
    string                    name;
    const TypeInfo*           ownerType = nullptr;
    const TypeInfo*           type = nullptr;

  public: // methods

    // Constructs given the field's name and pointer (global object).
    template <class FieldT>
    FieldInfo(string name_, FieldT* fieldPtr) :
      name(move(name_)),
      getFieldPointer([=](void*) -> void* { return fieldPtr; }),
      type(&TypeOf<FieldT>())
    {
    }

    // Constructs given the field's name and member pointer.
    template <class FieldT, class T>
    FieldInfo(string name_, FieldT T::*fieldPtr) :
      name(move(name_)),
      ownerType(&TypeOf<T>()),
      type(&TypeOf<FieldT>())
    {
      getFieldPointer = [=](void* this_) -> void*
      {
        return &(reinterpret_cast<T*>(this_)->*fieldPtr);
      };
    }

    // Returns a pointer to the field given its type.
    template <class FieldT>
    FieldT* Address() const
    {
      if (ownerType != nullptr) return nullptr;
      if (&TypeOf<FieldT>() != type) return nullptr;
      return reinterpret_cast<FieldT*>(getFieldPointer(nullptr));
    }

    // Returns a pointer to the field given its type
    //  and valid reference to the owning class.
    template <class FieldT, class T>
    FieldT* Address(T& this_) const
    {
      if (&TypeOf<T>() != ownerType) return nullptr;
      if (&TypeOf<FieldT>() != type) return nullptr;
      return reinterpret_cast<FieldT*>(getFieldPointer(&this_));
    }
  };
} // namespace lite