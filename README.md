# Table of Contents



- [Introduction](#introduction)
  - [Goals](#goals)
- [Tutorials](#tutorials)
  - [Implementing a Global Event System](#implementing-a-global-event-system)
  - [Implementing high_resolution_clock Correctly](#implementing-high_resolution_clock-correctly)
  - [Integrating FMOD Studio into a Game Project](#integrating-fmod-studio-into-a-game-project)
  - [Logging to the Windows Console in Color](#logging-to-the-windows-console-in-color)
  - [Rolling Your Own Variant](#rolling-your-own-variant)
- [Patterns](#patterns)
  - [Lightweight Singleton](#lightweight-singleton)
  - [Reference Member Initialization](#reference-member-initialization)
  - [Singleton](#singleton)
- [Miscellaneous](#miscellaneous)
  - [Ideas for Improvement](#ideas-for-improvement)
  - [Tutorial Goals](#tutorial-goals)



# Introduction


This project is made to be an instructive and lightweight example of a 3D game engine. The engine is simple: it is single threaded and the game loop exists in [Main.cpp](lite/Main.cpp). 

The engine is implemented entirely in headers, which allows for quick iteration time, and lets viewers learn about implementation details in-line with the class itself. If you have Visual Studio 2013 handy, I recommend you open up the solution [lite.sln](lite/lite.sln) and step into the various calls in [Main.cpp](lite/Main.cpp) to see what they do.



## Goals


1. To be simple and minimalistic.
2. To use modern C++ features effectively.
3. To be instructive on a wide range of features typically found in a 3D game engine.



# Tutorials


## Implementing a Global Event System


What is an event? Sometimes it's useful for systems to interact  with each other without having to directly access another. Consider keyboard input which requires messages from the window with WM_KEYDOWN messages. The input class could listen for an event invoked by the window when it receives a new message.

Usually it's useful to have an event *payload* so events can pass data around. Here is an example of an event payload which depends on [variants](#rolling-your-own-variant):

```C++
#include <string>
#include <unordered_map>
#include "Variant.hpp"

class EventData {
private: // data
  // Name of the event; useful for functions handling multiple events.
  string eventName;
  // Stores all possible data as a single hash-map.
  unordered_map<string, Variant> payload;
  // Friend EventSystem so it may set the eventName field.
  friend class EventSystem;

public: // methods
  // Allow the user to implement their own EventData.
  virtual ~EventData() {}

  // Returns whether the named data exists.
  bool Exists(const string& name) const { return payload.find(name) != payload.end(); }

  // Returns the data associated by name and type 'T'. The types must match exactly.
  template <class T>
  T& Get(const string& name) {
    Variant& variant = payload[name];
    FatalIf(!variant.IsValid(), "EventData::Get called with invalid name");

    // Assign a new type if the given type 'T' doesn't match the variant's type.
    if (!variant.IsType<T>()) variant.Assign<T>();

    return variant.Ref<T>();
  }

  // Returns the name of the event being called.
  const string& GetEventName() const { return eventName; }

  // Accesses the named data.
  Variant& operator[](const string& name) { return payload[name]; }
};
```

The variants allow the user to store any kind of data as payload. An alternative, more limited, approach would be to implement a union of various possible types.

A function capable of handling an event looks like this:

```C++
typedef std::function<void(EventData&)> EventHandlerFunction;
```

We store these functions in the `EventSystem` class:

```C++
#include <algorithm>
#include <utility>

class EventSystem : public Singleton<EventSystem> {
private: // data
  // Stores a handler function and its id.
  typedef pair<size_t, EventHandlerFunction> IdHandlerPair;
  // Stores all event handlers mapped by name.
  unordered_map<string, vector<IdHandlerPair>> eventHandlerMap;

public: // methods
  // Adds a handler given its event name, handler function, and an optional unique id.
  size_t AddHandler(const string& eventName, EventHandlerFunction fn, size_t id = GenerateHandlerId()) {
    eventHandlerMap[eventName].push_back(make_pair(id, move(fn)));
    return id;
  }

  // Returns whether the event exists in the system.
  bool Exists(const string& name) const { return eventHandlerMap.find(name) != eventHandlerMap.end(); }

  // Returns a unique id for event handlers.
  static size_t GenerateHandlerId() {
    static size_t id = 0;
    return id++;
  }
  
  // Removes an event handler given the event name and id.
  void RemoveHandler(const string& eventName, size_t id) {
    vector<IdHandlerPair>& handlerPairs = eventHandlerMap[eventName];

    // Find the handler matching the given id.
    auto it = find_if(
      handlerPairs.begin(),
      handlerPairs.end(),
      [&](IdHandlerPair& pair) { return pair.first == id; });
    if (it == handlerPairs.end()) return;

    // Erase the handler from the array.
    handlerPairs.erase(it);
  }
```

`Singleton<T>` looks like [this](#singleton). A handler identifier is stored so we can find which handler to remove when the object handling the event is destroyed. A lambda is sent into `find_if` to find the handler with the matching id. 

Invoking the event:

```C++
// Call an event with data.
size_t Invoke(string name, EventData& data) {
  if (!Exists(name)) return 0;

  // Get the map of event handlers, /then/ move 'name' into the EventData structure.
  auto& handlers = eventHandlerMap[name];
  data.eventName = move(name);

  // Call all handlers registered for this event. Called in reverse order so on-destruction events will clean up in the correct order.
  for (auto it = handlers.rbegin(); it != handlers.rend(); ++it) it->second(data); 

  return handlers.size();
}

// Call an event with no data.
size_t Invoke(string name) {
  EventData data;
  return Invoke(move(name), data);
}
```

We early-out if the event hasn't been registered. The handlers are called in reverse order to ensure that on-destruction messages are received in reverse order. The second `Invoke` is a convenience function in case no data is necessary.

That's essentially it--we have a global EventSystem which we can register event handlers with and invoke with a customized payload. There's one more cool thing we can do though. The user currently needs to call `AddHandler` on creation and `RemoveHandler` on destruction of their object in order to register and unregister their handler(s). We can create a wrapper object so the user doesn't need to call either.

```C++
class EventHandler {
private: // data
  // Name of the event being listened for.
  string eventName;
  // Unique id of this handler.
  size_t id = (size_t) -1;

public: // methods

  // Constructs the handler from an event name and function object.
  EventHandler(string eventName_, function<void(EventData&)> fn) 
    : eventName(move(eventName_)), id(EventSystem::GenerateHandlerId()) {
    EventSystem::Instance().AddHandler(eventName, move(fn), id);
  }

  // Constructs the handler from an event name, a 'this' pointer, and a pointer to the member function handling the event.
  template <class ThisType, class MemberFunctionPointer>
  EventHandler(string eventName_, ThisType* this_, MemberFunctionPointer memfn) :
    EventHandler(move(eventName_), [=](EventData& data) { (this_->*memfn)(data); }) {}

  // Cannot copy or move event handler because a previously captured 'this' pointer may be pointing to an invalid object after the copy.
  EventHandler() = delete;
  EventHandler(const EventHandler&) = delete;
  EventHandler& operator=(const EventHandler&) = delete;
  EventHandler(EventHandler&&) = delete;
  EventHandler& operator=(EventHandler&&) = delete;

  // Unregisters the event handler from the global event system.
  ~EventHandler() { Clear(); }

  // Unregisters the event handler from the global event system.
  void Clear() { EventSystem::Instance().RemoveHandler(eventName, id); }
};
```

The object registers the event handler with the global event system. The second constructor uses a lambda to store the `this` pointer and member function pointer in a closure. When the resulting `EventHandlerFunction` is called, the member function will be called. Copy constructor and assignment must be deleted because we don't want to copy the `this` pointer across objects; it must be reset every time.

This makes registering for a "WindowMessage" event easy:

```C++
class KeyboardBuffer {
private: // data
  // Automatically registers the OnWindowMessage member function with the "WindowMessage" event.
  EventHandler onWindowMessage = { "WindowMessage", this, &KeyboardBuffer::OnWindowMessage };
  
private: // methods
  // Handles the "WindowMessage" event.
  void OnWindowMessage(EventData&) {}
};
```

**Code Samples**
- [EventData](lite/EventData.hpp)
- [EventSystem](lite/EventSystem.hpp)
- [EventHandler](lite/EventHandler.hpp)

[Back to the table of contents.](#table-of-contents)



## Implementing high_resolution_clock Correctly



As of Visual Studio 2013, `std::chrono::high_resolution_clock` is still typedef'd to `system_clock` which is unacceptable to us as game developers. We need true high resolution times for profiling and getting accurate frame times.

```C++
#include <chrono>
#include <Windows.h>

class high_resolution_clock {
public: // types
  typedef double                                          rep;
  typedef std::ratio<1, 1>                                period;
  typedef std::chrono::duration<rep, period>              duration;
  typedef std::chrono::time_point<high_resolution_clock>  time_point;
  
public: // data
    // Whether the clock is non-decreasing.
    static const bool is_monotonic = true;
    // Whether the clock is monotonic and the time between clock ticks is constant.
    //  (The time between ticks is retrieved using QueryPerformanceFrequency.)
    static const bool is_steady = true;
};
```

We start off by defining some basic members that all clocks have. 

`rep` is the type representing the number of ticks. `period` is the tick period in seconds; `ratio<1,1>` means we'll be using seconds. `duration` is the difference between two times. `time_point` is measurement at a moment in time.

`is_monotonic` indicates whether the clock values are non-decreasing. `is_steady` indicates whether the time between ticks remains constant.

Let's establish a couple helper functions:

```C++
class high_resolution_clock {
// ...
private: // methods
  // Number of ticks per second.
  static int64_t frequency() {
    static int64_t freq = -1;
    if (freq == -1) QueryPerformanceFrequency((LARGE_INTEGER*)&freq); 
    return freq;
  }

  // Counter value at the start of the application.
  static int64_t start_count() {
    static int64_t count = 0;
    if (count == 0) QueryPerformanceCounter((LARGE_INTEGER*)&count);
    return count;
  }
};
```

`frequency` will call `QueryPerfomanceFrequency` for us to get the number of ticks per second. It'll cache the result so we only have to call `QueryPerformanceFrequency` once. `start_count` gives us the time near the start of the application. This is used in a subtraction from future calls to `QueryPerformanceCounter` to make the time value smaller. The int64_t value used to store time can store very large numbers that a double cannot. This way we can ensure the double is capable of holding the time value.

Now to the `time_point` function:

```C++
class high_resolution_clock {
// ...
public: // methods
  // Samples the current time.
  static time_point now() {
    // Get the current performance counter ticks.
    int64_t counter = 0;
    QueryPerformanceCounter((LARGE_INTEGER*)&counter);

    // Subtract the counter from the value at program start so a double can hold it.
    int64_t idt = counter - start_count();
    double dt = static_cast<double>(idt);

    return time_point(duration(dt / frequency()));
  }
// ...
};
```

The current time is retrieved using `QueryPerformanceCounter`. We then subtract `start_count` from that value to get a smaller value for the double. Then all you have to do to get the time in seconds is divide by `frequency`.

With a proper `high_resolution_clock`, we can trivially implement a stopwatch-like `high_resolution_timer`.

```C++
class high_resolution_timer {
private: // data
  // Time recorded when start() was last called.
  high_resolution_clock::time_point startTime;
  
public: // methods
  // Starts or restarts the timer.
  void start() {
    startTime = high_resolution_clock::now();
  }
};
```

`start` simply calls into `high_resolution_clock` to get the current time.

These functions help us get the current elapsed time:

```C++
class high_resolution_timer {
// ...
public: // methods
  // The current elapsed time as a duration object.
  high_resolution_clock::duration elapsed_duration() const {
    high_resolution_clock::time_point endTime = high_resolution_clock::now();
    return endTime - startTime;
  }
  
  // Microseconds that have elapsed since start() was called.
  double elapsed_microseconds() const {
    return duration_cast<duration<double, std::micro>>(elapsed_duration()).count();
  }
  
  // Milliseconds that have elapsed since start() was called.
  double elapsed_milliseconds() const {
    return duration_cast<duration<double, std::milli>>(elapsed_duration()).count();
  }
  
  // Seconds that have elapsed since start() was called.
  double elapsed_seconds() const {
    return elapsed_duration().count();
  }
// ...
};
```

`duration_cast` is capable of converting from seconds to milliseconds or any other time measurement.

This object will make implementing a [frame timer](lite/FrameTimer.hpp) fairly easy as well.

**[Code Sample](lite/chrono.hpp)**

[Back to the table of contents.](#table-of-contents)



## Integrating FMOD Studio into a Game Project



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

[Warn](#logging-macros) is a macro to report text in yellow to the console. You can replace this with a simple `assert()`, an exception, or any way you prefer to handle errors.

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

The for loop uses a custom helper class called `PathInfo` to get all files in a directory `config::Sounds`. You can simply read from a list of file names, roll your own method of doing this, or use Boost's [filesystem](http://www.boost.org/doc/libs/1_55_0/libs/filesystem/doc/index.htm).

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

**Code Samples**
  - [Audio](lite/Audio.hpp)
  - [EventDescription](lite/EventDescription.hpp)
  - [EventInstance](lite/EventInstance.hpp)

[Back to the table of contents.](#table-of-contents)



## Logging to the Windows Console in Color

If we have a class that buffers console output and prints in color, we can make our logging faster and easier to read.

```C++
class Console : public Singleton<Console> {
private: // data
  // Buffers output until a newline is encountered.
  string buffer;
  
public: // types
  // Color of the text being written to the console.
  enum Color {
    Black = 0,
    Blue = 0x1,
    Green = 0x2,
    Red = 0x4,
    White = Blue | Green | Red,
    Intensity = 0x8,
    Yellow = Red | Green,
    BrightRed = Red | Intensity,
    BrightYellow = Yellow | Intensity,
    BrightWhite = Red | Green | Blue | Intensity
  };
  
private: // methods
  Console() {
    // Create the console window.
    AllocConsole();
    MoveWindow(GetConsoleWindow(), 0, 0, 640, 850, TRUE);

    // Redirect stdout and stderr to the new console.
    FILE* file;
    freopen_s(&file, "CONOUT$", "w", stdout);
    freopen_s(&file, "CONOUT$", "w", stderr);
  }
  
  ~Console() {
    // Destroy the console window.
    FreeConsole();
  }
  
  // Let the singleton construct this object.
  friend Singleton<Console>;
```

[Singleton<T>](#singleton) is an implementation of the titular design pattern. The `Color` enum matches the colors provided by the Win32 API. The constructor creates the console, and moves it to the top left of the screen (optional). `freopen_s` (the safe version of `freopen`) lets us redirect `cout` and `cerr` to our newly created console. The destructor then destroys the console.

```C++
public: // methods
  // Prints a string to the console.
  void Print(const string& str) {
    buffer += str;

    // Flush if the last write ended with a newline or carriage-return.
    if (buffer.size() && (buffer.back() == '\n' || buffer.back() == '\r')) {
      buffer.pop_back(); // puts appends newline
      puts(buffer.c_str());
      buffer.clear();
    }
  }
```

Here we check the back of the string for a newline or carriage return. If either is found, we flush the buffer to the console.

```C++
  // Set the text color for writing to the console.
  void SetTextColor(Color c = BrightWhite) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), WORD(c));
  }

  // Change the text color.
  Console& operator<<(Color c) {
    SetTextColor(c);
    return *this;
  }

  // Writes a string to the console.
  Console& operator<<(const string& str) {
    Print(str);
    return *this;
  }

  // Writes any object to the console that supports stringstream insertion.
  template <class T>
  Console& operator<<(const T& object) {
    stringstream ss;
    ss << object;
    Print(ss.str());
    return *this;
  }
};
```

`SetTextColor` simply calls `SetConsoleTextAttibute` to set the color of console output. The templatized `operator<<` function will use `stringstream` to format the string before sending it into `Print`.



### Logging Macros



```C++
// Forms a scope that won't mess with calling code.
#define SCOPE(...) do { __VA_ARGS__; } while (0);

// Macro-ized if condition.
#define DO_IF(condition, ...) SCOPE( if (condition) { __VA_ARGS__; } )

// Enable macros/code only in debug mode.
#if defined(_DEBUG)
  #define DEBUG_ONLY(x) SCOPE(x)
#else
  #define DEBUG_ONLY(x) SCOPE()
#endif

// All log functions allow ostream-like insertion for formatting the string.

// Log fatal errors in bright red font.
#define Fatal(...)               DEBUG_ONLY(LogPrint(BrightRed, __VA_ARGS__); BREAKPOINT;)
#define FatalIf(condition, ...)  DEBUG_ONLY(DO_IF(condition, Fatal(__VA_ARGS__)))

// Print to the log in a specified color (see Console).
#define LogPrint(color, ...) DEBUG_ONLY(lite::Console::Instance() << lite::Console::color << __VA_ARGS__ << "\n")

// Log notes in white font.
#define Note(...)              DEBUG_ONLY(LogPrint(White, __VA_ARGS__))
#define NoteIf(condition, ...) DEBUG_ONLY(DO_IF(condition, Note(__VA_ARGS__)))

// Log warnings in yellow font.
#define Warn(...)              DEBUG_ONLY(static int _count = 0; if (++_count <= 3) { LogPrint(BrightYellow, __VA_ARGS__); } )
#define WarnIf(condition, ...) DEBUG_ONLY(DO_IF(condition, Warn(__VA_ARGS__)))
```

Each of these macros print in a different color to indicate the severity of each message. The `Warn` macros can only print up to 3 times. 

**[Code Sample](lite/Console.hpp)**

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



# Patterns



## Lightweight Singleton



Sometimes you want access to an object globally, but you know that another object will create it, so the usual Singleton implementation won't do.


```C++
// A lightweight singleton which uses the most recently created instance as the singleton.
template <class T>
struct LightSingleton {
  LightSingleton() { CurrentInstance() = static_cast<T*>(this); }
  ~LightSingleton() { CurrentInstance() = nullptr; }

  // Returns a pointer to the most recently created instance.
  static T*& CurrentInstance() {
    static T* instance = nullptr;
    return instance;
  }
};
```

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



## Singleton



The singleton design pattern can be implemented using the [curiously recurring template pattern](http://en.wikipedia.org/wiki/Curiously_recurring_template_pattern).

```C++
template <class T>
struct Singleton {
  static T& Instance() {
    static T instance;
    return instance;
  }
};
```

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



## Tutorial Goals



1. No fucking around. The text is there to detail all important points--maybe inspire some curiosity--but nothing more.
2. Most of the content is based in compact, working code samples.
3. Every tutorial comes with a working code sample and/or demo.

[Back to the table of contents.](#table-of-contents)
