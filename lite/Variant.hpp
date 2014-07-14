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

    // Used to compare the actual type of the variant (to be type-safe at runtime).
    type_index type = typeid(InvalidType);

  public: // methods

    Variant() = default;

    Variant(const Variant& b)
    {
      Assign(b);
    }

    Variant(Variant&& b)
    {
      Assign(move(b));
    }

    template <class T>
    Variant(T&& object)
    {
      Assign(forward<T>(object));
    }

    ~Variant() = default;

    Variant& operator=(const Variant& b)
    {
      return Assign(b);
    }

    Variant& operator=(Variant&& b)
    {
      return Assign(move(b));
    }

    template <class T>
    Variant& operator=(T&& object)
    {
      return Assign(forward<T>(object));
    }

    // Copies a variant.
    Variant& Assign(const Variant& b)
    {
      clone = b.clone;
      print = b.print;
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
      type = move(b.type);

      b.clone = nullptr;
      b.print = nullptr;
      b.type = typeid(InvalidType);

      return *this;
    }

    // Assigns a new object to the variant.
    template <class T>
    Variant& Assign(T&& object = T())
    {
      // Get the decayed type (no const or reference).
      typedef decay_t<T> DecayedType;

      // Verify that the class supports printing using the insertion operator.
      //  is_same allows us to check that the types of the two template parameters match.
      //  decltype gets the result of the given expression.
      //  declval creates a fake value that can be used within the expression. Here I'm
      //    using declval to pretend we already have a valid ostream&.
      //  If the type doesn't support the << operator the expression will fail, causing
      //    an error within the expression and then causing the static_assert 
      //    message to print.
      static_assert(
        is_same<ostream&, decltype(declval<ostream&>() << object)>::value, 
        "Objects wrapped in a Variant must support ostream printing using the insertion operator <<");

      // Copy the object if the types already match.
      if (data && type == typeid(T))
      {
        *reinterpret_cast<DecayedType*>(data.get()) = forward<T>(object);
        return *this;
      }

      // Get rid of the types for clone and print functions.
      clone = &CloneFunction<DecayedType>;
      print = &PrintFunction<DecayedType>;

      // Define the deleter using a lambda. Deletes the object as a 'T'
      //  type rather than a 'void'.
      auto deleter = [](void* p) { delete reinterpret_cast<DecayedType*>(p); };

      data = Pointer(new DecayedType(forward<T>(object)), move(deleter));
      type = typeid(DecayedType);

      return *this;
    }

    // Clears the variant; deleting the object data.
    void Clear()
    {
      clone = nullptr;
      data = Pointer(nullptr, nullptr);
      print = nullptr;
      type = typeid(InvalidType);
    }

    // A C-style function which generically allocates a copy of a given type.
    template <class T>
    static void* CloneFunction(void* otherThis)
    {
      return new T(*reinterpret_cast<T*>(otherThis));
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

    // Returns a reference to the variant's object.
    template <class T>
    T& Ref() const
    {
      return *Get<T>();
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

  };
} // namespace lite