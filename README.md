# Introduction

This library is made to be an instructive example of a 3D game engine built in C++ with Visual Studio and Direct3D 11. The engine is simple: it is single threaded and the game loop exists in Main.cpp.

## Goals

1. To be simple and minimalistic.

2. To use modern C++ features effectively.

3. To be instructive on a wide range of features typically found in a 3D game engine.

# Tutorials

## Audio

### How to Use FMOD Studio for a Game Project

Include the FMOD Studio headers and Visual C++ static library:

```C++
// Headers from the FMOD Studio API "api/studio/include"
#include "FMOD/fmod_errors.h"
#include "FMOD/fmod_studio.hpp"

// Link the library from "api/studio/lib"
#pragma comment(lib, "FMOD/fmodstudio_vc.lib")
```

I also add to my FmodInclude.hpp header these things (optional):

```C++
namespace lite {
  namespace fmod = FMOD::Studio; // alias for convenience
}

// Checks the result of an FMOD call for errors.
#define FmodCall(x, ...) do { \
    FMOD_RESULT fmodResult; \
    if ((fmodResult = (x)) != FMOD_OK) \
    { \
      Warn("FMOD error: (" << int(fmodResult) << ") " << #x << "\n" << FMOD_ErrorString(fmodResult)); \
      return __VA_ARGS__; \
    } \
  } while(0)
```

Let's start the Audio system:

```C++
class Audio {
private: // data
  // Maximum number of channels allowed on the system.
  static const int maxChannels = 512;
  // Fmod Studio's system interface.
  fmod::System* system;
  
public: // methods
  Audio();
};
```

The constructor creates the `FMOD::Studio::System` object.

```C++
Studio() {
  // Create the system.
  FmodCall(fmod::System::create(&system));
  // Initialize the system.
  FmodCall(system->initialize(
    maxChannels,               // max channels capable of playing audio
    FMOD_STUDIO_INIT_NORMAL,   // studio-specific flags
    FMOD_INIT_3D_RIGHTHANDED,  // regular flags
    nullptr));                 // extra driver data
}
```

The `fmod::System::create` call simply fills in the `System*` with a valid object. 

The initialize call tells FMOD Studio how many channels we need (maximum). This is essentially the number of sounds capable of being played at the same time. 512 should be plenty. `FMOD_STUDIO_INIT_NORMAL` does the default FMOD Studio initialization, which works fine for our purposes. `FMOD_INIT_3D_RIGHTHANDED` changes the 3D coordinate system to be compatible with Direct3D. You can use `FMOD_INIT_NORMAL` here if you're using OpenGL.

We have a system successfully initializing, so let's load in some sound banks. Here's the `SoundBank` wrapper class:

```C++
class SoundBank {
private: // data
  fmod::Bank* bank;
  string      name;
  
public: // methods
  // Assigns private data.
  SoundBank(fmod::Bank* bank, string name);
  // Returns an array of all events in the bank.
  vector<fmod::EventDescription*> GetEventList() const;
};
```

The `GetEventList` function gives us an array of all events in the sound bank we can use to play. The implementation looks something like this:

```C++
vector<fmod::EventDescription*> GetEventList() const {
  FatalIf(bank == nullptr, "GetEventList: Bank is null");

  // Get the number of events in the sound bank.
  int eventCount = 0;
  FmodCall(bank->getEventCount(&eventCount), {});
  if (eventCount == 0) return {};

  // Get the event list from the FMOD bank.
  auto result = vector<fmod::EventDescription*>(static_cast<size_t>(eventCount), nullptr);
  FmodCall(bank->getEventList(result.data(), result.size(), &eventCount), {});

  return move(result);
}
```

The event count is retrieved from `bank->getEventCount` which is used to size the array sent to `bank->getEventList`.

In the Audio class we can have a map of SoundBank objects.

```C++
#include <unordered_map>

class Audio {
private: // data
// ...
  // Map of bank names to sound bank objects.
  unordered_map<string, SoundBank> soundBankMap;
// ...
};
```

Continuing the constructor implementation, we can load in all available sound banks:

```C++
Audio() {
// ...
  // For each file in the Sounds directory with a *.bank extension:
  for (string& file : PathInfo(config::Sounds).FilesWithExtension("bank")) {
    // Load the sound bank from file.
    fmod::Bank* bank = nullptr;
    FmodCall(system->loadBankFile(file.c_str(), FMOD_STUDIO_LOAD_BANK_NORMAL, &bank));

    // Get the path name of the sound bank.
    auto path = string(512, ' ');
    int pathLen = 0;
    FmodCall(bank->getPath(&path[0], static_cast<int>(path.size()), &pathLen));
    path.resize(static_cast<size_t>(pathLen - 1));

    // Add a map entry to the sound bank using the bank's path name.
    soundBankMap.emplace(path, SoundBank(bank, path));
  }
}
```

The for loop uses a custom helper class called `PathInfo` to get all files in a directory `config::Sounds`. You can roll your own method of doing this, read from a list of file names, or use Boost's [filesystem](http://www.boost.org/doc/libs/1_55_0/libs/filesystem/doc/index.htm).

The sound bank is loaded using `system->loadBankFile` with default loading. The path name of the bank is retrieved using `bank->getPath`. `soundBankMap.emplace` adds a path and SoundBank pair into the map.

## Generic Utilities

### How to Roll Your Own Variant Class

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
  
  // Forward the object argument into the constructor of the new object.
  data = Pointer(new DecayT(forward<T>(object)), move(deleter));
}
```

Lambdas provide a convenient method of erasing the types by creating generic functions that only deal with void pointers. 

The clone takes in a `const void*` representing the object to copy. We `reinterpret_cast` that pointer to the correct type in order to construct the new object. We then return the allocated copy as a `void*`. The deleter is casting its `void*` parameter to ensure that `delete` calls the destructor on the object.


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

If the variant we're copying from stores an object we'll call the clone function to copy construct a new object.  The deleter is also passed along to the new `unique_ptr`.

We can add some functions to deal with the variant's data.

```C++
// Whether the variant is valid (non-null).
explicit operator bool() const { return bool(data); }

// Whether the variant is invalid (null).
bool operator!() const { return !bool(*this); }

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
