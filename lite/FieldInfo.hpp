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
      getFieldPointer(move(b.getFieldPointer)),
      getter(move(b.getter)),
      name(move(b.name)),
      ownerType(b.ownerType),
      setter(move(b.setter)),
      type(b.type)
    {}

    FieldInfo& operator=(FieldInfo&& b)
    {
      getFieldPointer = move(b.getFieldPointer);
      getter = move(b.getter);
      name = move(b.name);
      ownerType = b.ownerType;
      setter = move(b.setter);
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

      getter = [=](void* this_) -> Variant
      {
        return (reinterpret_cast<T*>(this_)->*fieldPtr);
      };

      setter = [=](void* this_, void* other)
      {
        FieldT& field = (reinterpret_cast<T*>(this_)->*fieldPtr);
        field = *reinterpret_cast<FieldT*>(other);
      };
    }

    // Constructs a field info from a getter.
    template <class FieldT1, class T1>
    FieldInfo(string name_, FieldT1(T1::*getter)() const) :
      name(move(name_)),
      ownerType(&TypeOf<T1>()),
      type(&TypeOf<FieldT1>())
    {
      // Create the generic getter function. Takes the 'this' pointer as a void*
      //  and casts it to the class type. It calls the getter function and returns
      //  the result as a variant.
      this->getter = [=](void* this_) -> Variant
      {
        return (reinterpret_cast<T1*>(this_)->*getter)();
      };
    }

    // Constructs a field info from a getter.
    template <class FieldT1, class T1>
    FieldInfo(string name_, FieldT1&(T1::*getter)() const) :
      name(move(name_)),
      ownerType(&TypeOf<T1>()),
      type(&TypeOf<FieldT1>())
    {
      // Create the generic getter function. Takes the 'this' pointer as a void*
      //  and casts it to the class type. It calls the getter function and returns
      //  the result as a variant.
      this->getter = [=](void* this_) -> Variant
      {
        return &(reinterpret_cast<T1*>(this_)->*getter)();
      };
    }

    // Constructs a field info from a getter / setter pair.
    template <class FieldT1, class FieldT2, class T1, class T2>
    FieldInfo(string name_, FieldT1(T1::*getter)() const, void(T2::*setter)(FieldT2)) :
      name(move(name_)),
      ownerType(&TypeOf<T1>()),
      type(&TypeOf<FieldT1>())
    {
      // Create the generic getter function. Takes the 'this' pointer as a void*
      //  and casts it to the class type. It calls the getter function and returns
      //  the result as a variant.
      this->getter = [=](void* this_) -> Variant
      {
        return (reinterpret_cast<T1*>(this_)->*getter)();
      };

      // Create the generic setter function. Takes the 'this' pointer as a void*
      //  and the value as a void* and casts the pointers to their appropriate
      //  types prior to calling the set function.
      this->setter = [=](void* this_, void* value)
      {
        (reinterpret_cast<T2*>(this_)->*setter)(*reinterpret_cast<decay_t<FieldT2>*>(value));
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

    Variant Get(void* this_ = nullptr) const
    {
      if (this == nullptr && ownerType != nullptr)
      {
        Warn("'this' pointer required for field " << Name);
        return {};
      }
      return getter(this_);
    }

    void Set(const Variant& value, void* this_ = nullptr) const
    {
      if (this_ == nullptr && ownerType != nullptr)
      {
        Warn("'this' pointer required for field " << Name);
        return;
      }
      setter(this_, value.Data());
    }

    // Formats the field info into an ostream.
    friend ostream& operator<<(ostream& os, const FieldInfo& f);
  };

  template <class ClassT, class RetT, class... Args, class FuncPtr = RetT(ClassT::*)(Args...) const>
  FuncPtr Const(RetT(ClassT::*fn)(Args...) const)
  {
    return fn;
  }

  static struct ReadOnly_* ReadOnly = nullptr;

  template <class ClassT, class RetT, class... Args, class FuncPtr = RetT(ClassT::*)(Args...)>
  FuncPtr NonConst(RetT(ClassT::*fn)(Args...))
  {
    return fn;
  }

  static struct WriteOnly_* WriteOnly = nullptr;
} // namespace lite