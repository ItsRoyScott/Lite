#pragma once

#include "Essentials.hpp"

namespace lite
{
  // Stores an object of any type.
  class Variant
  {
  public: // types

    typedef void*(*CloneFunction)(const void*);
    typedef ostream&(*PrintFunction)(ostream&, const void*);
    typedef istream&(*ReadFunction)(istream&, void*);

  private: // types

    // Invalid / unassigned type.
    struct InvalidType {};

    // Smart pointer which stores the variant's data.
    typedef unique_ptr<void, void(*)(void*)> Pointer;

  private: // data

    // Allocates a new copy of the object.
    CloneFunction clone = nullptr;

    // Stores our data as a void pointer.
    Pointer data = Pointer(nullptr, nullptr);

    // Prints the object using the given ostream.
    PrintFunction print = nullptr;

    // Reads the object using the given istream.
    ReadFunction read = nullptr;

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
        // Use clone to create a new object and copy the deleter.
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
    Variant& Assign(const T& object = T())
    {
      SetType<T>();

      typedef decay_t<T> DecayedT;

      // Copy the object if the types already match.
      if (data && type == typeid(DecayedT))
      {
        DecayedT* ptr = reinterpret_cast<DecayedT*>(data.get());
        ptr->~DecayedT();
        new (ptr) DecayedT(object);
        return *this;
      }

      // Define the deleter using a lambda. Deletes the object as a 'T'
      //  type rather than a 'void'.
      auto deleter = [](void* p) { delete reinterpret_cast<DecayedT*>(p); };

      data = Pointer(new DecayedT(object), move(deleter));

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

    // Retrieves the internal void pointer.
    void* Data() const
    {
      return data.get();
    }

    // Generates a generic function which inserts the given type into an ostream object.
    //  The second template parameter is used to check if the type supports insertion.
    //  If the decltype fails, the other version of this function will be called.
    template <class T, class = decltype(new T(declval<const T&>()))>
    static CloneFunction GenerateCloneFunction()
    {
      return [](const void* other) -> void*
      {
        return new T(*reinterpret_cast<const T*>(other));
      };
    }

    // Generates a generic function which simply returns ostream for a type lacking
    //  support for ostream insertion. The ellipses in the function call make this
    //  function have the lowest priority for candidate functions. (The compiler
    //  will prefer any function but this one.)
    template <class T>
    static CloneFunction GenerateCloneFunction(...)
    {
      return [](const void*) -> void* { return nullptr; };
    }

    // Generates a generic function which inserts the given type into an ostream object.
    //  The second template parameter is used to check if the type supports insertion.
    //  If the decltype fails, the other version of this function will be called.
    template <class T, class = decltype(declval<ostream&>() << declval<const T&>())>
    static PrintFunction GeneratePrintFunction()
    {
      return [](ostream& os, const void* this_) -> ostream& 
      { 
        return os << *reinterpret_cast<const T*>(this_); 
      };
    }

    // Generates a generic function which simply returns ostream for a type lacking
    //  support for ostream insertion. The ellipses in the function call make this
    //  function have the lowest priority for candidate functions. (The compiler
    //  will prefer any function but this one.)
    template <class T>
    static PrintFunction GeneratePrintFunction(...)
    {
      return [](ostream& os, const void*) -> ostream& { return os; };
    }

    // Generates a generic function which extracts the given type from an istream object.
    //  The second template parameter is used to check if the type supports extraction.
    //  If the decltype fails, the other version of this function will be called.
    template <class T, class = decltype(declval<istream&>() >> declval<T&>())>
    static ReadFunction GenerateReadFunction()
    {
      return [](istream& os, void* this_) -> istream&
      {
        return os >> *reinterpret_cast<T*>(this_);
      };
    }

    // Generates a generic function which simply returns istream for a type lacking
    //  support for istream insertion. The ellipses in the function call make this
    //  function have the lowest priority for candidate functions. (The compiler
    //  will prefer any function but this one.)
    template <class T>
    static ReadFunction GenerateReadFunction(...)
    {
      return [](istream& os, void*) -> istream& { return os; };
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

    // Returns a reference to the variant's object.
    template <class T>
    T& Ref() const
    {
      FatalIf(!IsType<T>(), "Attempting to reference variant type " << type.name() << " as " << typeid(T).name());
      return *Get<T>();
    }

    template <class T>
    Variant& SetType()
    {
      Clear();

      // Get rid of the types for clone and print functions.
      clone = GenerateCloneFunction<decay_t<T>>();
      print = GeneratePrintFunction<decay_t<T>>();
      read = GenerateReadFunction<decay_t<T>>();

      type = typeid(decay_t<T>);

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
      return Ref<T>();
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