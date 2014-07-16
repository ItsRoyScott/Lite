# Table of Contents

- [Introduction](#introduction)
  - [Goals](#goals)
- [Tutorials](#tutorials)
  - [Importing FMOD Studio into a Game Project](#importing-fmod-studio-into-a-game-project)
  - [Rolling Your Own Variant](#rolling-your-own-variant)
- [Miscellaneous](#miscellaneous)
  - [Ideas for Improvement](#ideas-for-improvement)
  - [Reference Member Initialization](#reference-member-initialization)

# Introduction

This project is made to be an instructive and lightweight example of a 3D game engine. The engine is simple: it is single threaded and the game loop exists in [Main.cpp](lite/Main.cpp). 

The engine is implemented entirely in headers, which allows for quick iteration time, and lets viewers learn about implementation details in-line with the class itself. If you have Visual Studio 2013 handy, I recommend you open up the solution [lite.sln](lite/lite.sln) and step into the various calls in [Main.cpp](lite/Main.cpp) to see what they do.

## Goals

1. To be simple and minimalistic.

2. To use modern C++ features effectively.

3. To be instructive on a wide range of features typically found in a 3D game engine.

# Tutorials



## Importing FMOD Studio into a Game Project



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

The `initialize` call tells FMOD Studio how many channels we need (maximum). This is essentially the number of sounds capable of being played at the same time. 512 should be plenty. `FMOD_STUDIO_INIT_NORMAL` does the default FMOD Studio initialization, which works fine for our purposes. `FMOD_INIT_3D_RIGHTHANDED` changes the 3D coordinate system to be compatible with Direct3D. You can use `FMOD_INIT_NORMAL` here if you're using OpenGL.

FMOD Studio allows playing sounds through events. Every event has a path string used to identify the event. The event can have user-defined parameters that the sound engineer specifies in FMOD Studio, where he or she can tell designers/programmers when and how to play the sound.

A wrapper class for `FMOD::Studio::EventDescription` will give us easy access to the underlying event description.

```C++
class EventDescription {
private: // data
  // Pointer to FMOD's event description.
  fmod::EventDescription* description;
  // The path string to the description.
  string path;
  
public: // methods
  // Assigns private data.
  EventDescription(fmod::EventDescription* description_, string path_);
};
```

We add a map of these to the Audio system:

```C++
class Audio {
// ...
  // Map of event names to their description objects.
  unordered_map<string, EventDescription> eventDescriptionMap;
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

    // Get the number of events in the bank.
    int eventCount = 0;
    FmodCall(bank->getEventCount(&eventCount));

    // Get the list of event descriptions from the bank.
    auto eventArray = vector<fmod::EventDescription*>(static_cast<size_t>(eventCount), nullptr);
    FmodCall(bank->getEventList(eventArray.data(), eventArray.size(), nullptr));
```

The for loop uses a custom helper class called `PathInfo` to get all files in a directory `config::Sounds`. You can roll your own method of doing this, read from a list of file names, or use Boost's [filesystem](http://www.boost.org/doc/libs/1_55_0/libs/filesystem/doc/index.htm).

The sound bank is loaded using `system->loadBankFile` with default loading. We then get the event count and an array of event descriptions to initialize our map of `EventDescription` objects:

```C++
    // For each event description:
    for (fmod::EventDescription* eventDescription : eventArray) {
      // Get the path to the event, e.g. "event:/Ambience/Country"
      auto path = string(512, ' ');
      int retrieved = 0;
      FmodCall(eventDescription->getPath(&path[0], path.size(), &retrieved));
      path.resize(static_cast<size_t>(retrieved - 1)); // - 1 to account for null character

      // Save the event description in the event map.
      eventDescriptionMap.emplace(path, EventDescription(eventDescription, path));
    }
  }
}
```

The path to the event is retrieved using `getPath` and we resize the string to account for the actual size of the path. Then we add the event description to the map using `emplace`.

When an event is played it spawns an `EventInstance`. We can create a wrapper class for this too:

```C++
class EventInstance {
private: // data
  // The event description corresponding to this event.
  EventDescription* description = nullptr;
  // The FMOD event instance.
  fmod::EventInstance* instance = nullptr;
  
public: // data
  // 3D attributes
  FMOD_VECTOR Forward   = FMOD_VECTOR{ 0, 0, 1 };
  FMOD_VECTOR Position  = FMOD_VECTOR{ 0, 0, 0 };
  FMOD_VECTOR Up        = FMOD_VECTOR{ 0, 1, 0 };
  FMOD_VECTOR Velocity  = FMOD_VECTOR{ 0, 0, 0 };
```

**[Code Sample](lite/Audio.hpp)**

[Back to the table of contents.](#table-of-contents)



## Rolling Your Own Variant



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

If the variant we're copying from stores an object, we'll call the clone function to copy construct a new object.  The deleter is also passed along to the new `unique_ptr`.

We can add some functions to deal with the variant's internal data:

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

**[Code Sample](lite/Variant.hpp)**

[Back to the table of contents.](#table-of-contents)

# Miscellaneous



## Ideas for Improvement



- Separate thread for rendering, i.e. "Render Thread".
- Re-save converted models for faster loading.
- Better logging mechanism: message box, log-to-file, filename / line, channels, in-game, etc.
- A performance profiler.
- A resource manager which handles all resource loading (possibly on a separate thread).
- Xbox or Wii controller input.
- Action mappings for input: "Jump" maps to VK_SPACE, etc.
- Separate thread or threadpool for physics.

[Back to the table of contents.](#table-of-contents)



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

[Back to the table of contents.](#table-of-contents)
