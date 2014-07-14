#pragma once

#include "Essentials.hpp"
#include "ReflectionUtility.hpp"

namespace lite
{
  // Meta information on a field in C++.
  class FieldInfo
  {
  private: // data

    function<Variant(void*)>      getter;
    function<void*(void*)>        getFieldPointer;
    string                        name;
    const TypeInfo*               ownerType = nullptr;
    function<void(void*, void*)>  setter;
    const TypeInfo*               type = nullptr;

  public: // properties

    // Name of the field.
    const string& Name = name;

    // Class that owns this field (may be null for global/static objects).
    const TypeInfo* const& OwnerType = ownerType;

    // Type of the field.
    const TypeInfo* const& Type = type;

  public: // methods

    FieldInfo() = delete;

    FieldInfo(FieldInfo&& b) :
      name(move(b.name)),
      ownerType(b.ownerType),
      type(b.type)
    {}

    FieldInfo& operator=(FieldInfo&& b)
    {
      name = move(b.name);
      ownerType = b.ownerType;
      type = b.type;

      return *this;
    }

    FieldInfo(const FieldInfo&) = delete;
    FieldInfo& operator=(const FieldInfo&) = delete;

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
        FieldT& field = (reinterpret_cast<T*>(this_)->*fieldPtr);
        return &field;
      };
    }

    // Constructs a field info from a getter / setter pair.
    //  These are typical getters and setters, the only restriction being
    //  that the getter must be const.
    template <class FieldT1, class FieldT2, class T1, class T2>
    FieldInfo(string name_, FieldT1(T1::*getter)() const, void(T2::*setter)(FieldT2)) :
      name(move(name_)),
      ownerType(&TypeOf<T1>()),
      type(&TypeOf<FieldT1>())
    {
      // Get the decayed types. std::decay removes references and const so a const T&
      //  would be simply T.
      typedef decay_t<T1> T1Value;
      typedef decay_t<T2> T2Value;
      typedef decay_t<FieldT1> FieldT1Value;
      typedef decay_t<FieldT2> FieldT2Value;
      static_assert(is_same<T1Value, T2Value>::value, "Class types for getters and setters must match");
      static_assert(is_same<FieldT1Value, FieldT2Value>::value, "Field types for getters and setters must match");

      this->getter = [=](void* this_) -> Variant
      {
        return (reinterpret_cast<T1Value*>(this_)->*getter)();
      };

      this->setter = [=](void* this_, void* value)
      {
        (reinterpret_cast<T2Value*>(this_)->*setter)(*reinterpret_cast<FieldT2Value*>(value));
      };
    }

    ~FieldInfo() = default;

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

    // Formats the field info into an ostream.
    friend ostream& operator<<(ostream& os, const FieldInfo& f);
  };

  template <class GetClassT, class GetRetT>
  auto Getter(GetRetT(GetClassT::*fn)() const) -> GetRetT(GetClassT::*)() const
  {
    return fn;
  }

  template <class SetClassT, class SetArgT>
  auto Setter(void(SetClassT::*fn)(SetArgT)) -> void(SetClassT::*)(SetArgT)
  {
    return fn;
  }
} // namespace lite