#pragma once

#include "Essentials.hpp"

namespace lite
{
  // Stores an object of any type.
  class Variant
  {
  private: // types

    // Invalid / unassigned type.
    struct InvalidType {};

    // Smart pointer which stores the variant's data.
    typedef unique_ptr<void, void(*)(void*)> Pointer;

  private: // data

    // Allocates a new copy of the object.
    void* (*clone)(void* other) = nullptr;

    // Stores our data as a void pointer.
    Pointer data = Pointer(nullptr, nullptr);

    // Prints the object using the given ostream.
    ostream& (*print)(ostream& os, void* p) = nullptr;

    // Reads the object using the given istream.
    istream& (*read)(istream& is, void* p) = nullptr;

    // Used to compare the actual type of the variant (to be type-safe at runtime).
    type_index type = typeid(InvalidType);

  public: // methods

    Variant() = default;

    Variant(const Variant& b)
    {
      Assign(b);
    }

    Variant& operator=(const Variant& b)
    {
      return Assign(b);
    }

    Variant(Variant&& b)
    {
      Assign(move(b));
    }

    Variant& operator=(Variant&& b)
    {
      return Assign(move(b));
    }

    template <class T>
    Variant(T&& object)
    {
      Assign(forward<T>(object));
    }

    template <class T>
    Variant& operator=(T&& object)
    {
      return Assign(forward<T>(object));
    }

    ~Variant() = default;

    // Copies a variant.
    Variant& Assign(const Variant& b)
    {
      clone = b.clone;
      print = b.print;
      read = b.read;
      type = b.type;
      if (b.data)
      {
        data = Pointer(clone(b.data.get()), b.data.get_deleter());
      }

      return *this;
    }

    // Moves a variant.
    Variant& Assign(Variant&& b)
    {
      clone = move(b.clone);
      data = move(b.data);
      print = move(b.print);
      read = move(b.read);
      type = move(b.type);

      b.clone = nullptr;
      b.print = nullptr;
      b.read = nullptr;
      b.type = typeid(InvalidType);

      return *this;
    }

    // Assigns a new object to the variant.
    template <class T>
    Variant& Assign(T&& object = T())
    {
      SetType<T>();

      typedef decay_t<T> DecayedT;

      // Copy the object if the types already match.
      if (data && type == typeid(T))
      {
        DecayedT* ptr = reinterpret_cast<DecayedT*>(data.get());
        ptr->~DecayedT();
        new (ptr) DecayedT(forward<T>(object));
        return *this;
      }

      // Define the deleter using a lambda. Deletes the object as a 'T'
      //  type rather than a 'void'.
      auto deleter = [](void* p) { delete reinterpret_cast<DecayedT*>(p); };

      data = Pointer(new DecayedT(forward<T>(object)), move(deleter));

      return *this;
    }

    // Clears the variant; deleting the object data.
    void Clear()
    {
      clone = nullptr;
      data = Pointer(nullptr, nullptr);
      read = nullptr;
      print = nullptr;
      type = typeid(InvalidType);
    }

    // A C-style function which generically allocates a copy of a given type.
    template <class T>
    static void* CloneFunction(void* otherThis)
    {
      return new T(*reinterpret_cast<T*>(otherThis));
    }

    // Retrieves the internal void pointer.
    void* Data() const
    {
      return data.get();
    }

    // Retrieves the object the variant is pointing to. 
    //  Type 'T' must match the variant's type.
    template <class T>
    T* Get() const
    {
      if (!IsType<T>()) return nullptr;
      return reinterpret_cast<T*>(data.get());
    }

    // Returns whether the type 'T' matches the variant's type.
    template <class T>
    bool IsType() const
    {
      return type == typeid(T);
    }

    // Returns whether the variant currently stores data.
    bool IsValid() const
    {
      return bool(data);
    }

    // A C-style function which is capable of printing a given type to an ostream.
    template <class T>
    static ostream& PrintFunction(ostream& os, void* this_)
    {
      return os << *reinterpret_cast<T*>(this_);
    }

    template <class T>
    static istream& ReadFunction(istream& is, void* this_)
    {
      return is >> *reinterpret_cast<T*>(this_);
    }

    // Returns a reference to the variant's object.
    template <class T>
    T& Ref() const
    {
      return *Get<T>();
    }

    template <class T>
    Variant& SetType()
    {
      Clear();

      // Get the decayed type (no const or reference).
      typedef decay_t<T> DecayedType;

      // Get rid of the types for clone and print functions.
      clone = &CloneFunction < DecayedType > ;
      print = &PrintFunction < DecayedType > ;
      read = &ReadFunction < DecayedType > ;

      type = typeid(DecayedType);

      return *this;
    }

    // Whether the variant is valid (non-null).
    explicit operator bool() const
    {
      return bool(data);
    }

    // Whether the variant is invalid (null).
    bool operator!() const
    {
      return !bool(*this);
    }

    // Returns a copy of the variant's data by-value.
    template <class T>
    explicit operator T() const
    {
      return *Get<T>();
    }

    // Prints the variant to an ostream object.
    friend ostream& operator<<(ostream& os, const Variant& v)
    {
      if (!v.data) return os;
      return v.print(os, v.data.get());
    }

    // Reads the variant from an istream object.
    friend istream& operator>>(istream& is, Variant& v)
    {
      if (!v.data) return is;
      return v.read(is, v.data.get());
    }
  };
} // namespace lite