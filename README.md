# Introduction

This library is made to be an instructive example of a 3D game engine built in C++ with Visual Studio and Direct3D 11. The engine is simple: it is single threaded and the game loop exists in Main.cpp.

## Goals

1. To be simple and minimalistic.

2. To use modern C++ features effectively.

3. To be instructive on a wide range of features typically found in a 3D game engine.

# Tutorials

## Generic Utilities

### Rolling Your Own Variant Class

A `variant`, also known as an `any`, can store any C++ object inside it. Its data members are a void pointer to the object data, a `clone` function pointer which copies the data, a deleter function which destroys the data, and type info used for error checking.

```C++
#include <memory>
#include <typeinfo>

class Variant {
private: // types
  // Type indicating an unassigned variant.
  struct InvalidType {};
  // unique_ptr allows us to store the data and its deleter function.
  typedef unique_ptr<void, void(*)(void*)> Pointer;
  
private: // data
  // Uses 'new' to allocate and copy construct the object.
  void* (*clone)(const void* other) = nullptr;
  // Stores our data as a void pointer.
  Pointer data = Pointer(nullptr, nullptr);
  // Used to compare the actual type of the variant (to be type-safe at runtime).
  type_index type = typeid(InvalidType);
};
```

With this we can create a templatized constructor accepting any type.

```C++
class Variant {
// Same As Above
public: // methods
  template <class T>
  Variant(T&& object);
```

The double-ampersand makes the parameter a universal reference. (See [Scott Meyers' post on universal references] (https://isocpp.org/blog/2012/11/universal-references-in-c11-scott-meyers))

```C++
Variant(T&& object) {
  // Decay the type; this removes references and const.
  typedef decay_t<T> DecayT;
  type = typeid(DecayT);
  
  // For clone we have to cast the parameter to the proper type.
  clone = [](const void* other) -> void* { return new DecayT(*reinterpret_cast<const DecayT*>(other)); };
  
  // For the deleter function we have to make sure we're deleting the proper type.
  auto deleter = [](void* p) { delete reinterpret_cast<DecayT*>(p); };
  
  // Forward the given object into the constructor of the new object.
  data = Pointer(new DecayT(forward<T>(object)), move(deleter));
}
```

Lambdas provide a convenient method of erasing the types by creating generic functions that only deal with void pointers.

Copy the internal object when the variant is copied:

```C++
Variant(const Variant& b) {
  clone = b.clone;
  type = b.type;
  if (b.data) {
    data = Pointer(clone(b.data.get()), b.data().get_deleter());
  }
}
```

If the variant we're copying from stores an object we'll call the clone function to copy construct a new object. 

We can add some functions to deal with the variant's data.

```C++
// Whether the variant is valid (non-null).
explicit operator bool() const { return bool(data); }

// Whether the variant is invalid (null).
explicit operator!() const { return !bool(*this); }

// Whether the type 'T' matches the variant's type.
template <class T>
bool IsType() const { return type == typeid(T); }

// Returns a pointer to the object (null if the type doesn't match).
template <class T>
T* Get() const {
  if (!IsType<T>()) return nullptr;
  return reinterpret_cast<T*>(data.get());
}
```

There's more we can do with Variant:
- Store function pointers to generically format to an `ostream` or read from an `istream`.
- Use a `char buffer[]` to avoid dynamic allocations for small objects.
- Interact with reflection to let the user explore fields and methods on the object.
- Support implicit casts (like `short` -> `int`).
- Treat references and pointers specially so the user can get the underlying type.

# Miscellaneous

## Reference Member Initialization

Some objects in Lite use member initialization at the declaration (C++11 feature) to initialize a reference to a private member for public access.

```C++
class TypeInfo {
private: // data
  string name;
public: // properties
  const string& Name = name;
};
```

The problem with this approach to exposing attributes is that the compiler cannot generate a default copy constructor or assignment operator. The reference is effectively a pointer to a field of the same object, so we do not want to copy the pointer to the next object. 

What we want to have happen is for the reference member initialization to be left alone on construction and assignment. 

```C++
class TypeInfo { 
// Same As Above
public: // methods
  TypeInfo(const TypeInfo&) {}
  TypeInfo& operator=(const TypeInfo&) { return *this; }
};
```

This leaves a little more work for the programmer if the copy constructor and assignment were not needed. It's up to you if the benefit of saying `typeInfo.Name` outweighs a simple member function `typeInfo.Name()`.

## Ideas for Improvement

- Separate thread for rendering, i.e. "Render Thread".

- Re-save converted models for faster loading.

- Better logging mechanism: message box, log-to-file, filename / line, channels, in-game, etc.

- A performance profiler.

- A resource manager which handles all resource loading (possibly on a separate thread).

- Xbox or Wii controller input.

- Action mappings for input: "Jump" maps to VK_SPACE, etc.

- Separate thread or threadpool for physics.
