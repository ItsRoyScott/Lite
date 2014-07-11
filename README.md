# Introduction

I built this library as an instructive example game engine for Game Engine Architecture at DigiPen. The engine is simple: it is single threaded and the game loop exists in Main.cpp.

## Goals

1. To be simple and minimalistic.

2. To use modern C++ features effectively.

3. To be instructive on a wide range of features typically found in a 3D game engine.

## Ideas for improvement

- Separate thread for rendering, i.e. "Render Thread".

- Re-save converted models for faster loading.

- Better logging mechanism: message box, log-to-file, filename / line, channels, in-game, etc.

- A performance profiler.

- A resource manager which handles all resource loading (possibly on a separate thread).

- Xbox or Wii controller input.

- Action mappings for input: "Jump" maps to VK_SPACE, etc.

# Topics

## Reference member initialization

Some objects in Lite use member initialization at the declaration (C++11 feature) to initialize a reference to a private member for public access.

```C++
class TypeInfo {
private: // data
  string name;
public: // properties
  const string& Name = name;
};
```

The problem with this approach to properties or attributes is that the compiler cannot generate a default copy constructor or assignment operator.The reference is effectively a pointer to a field of the same object, so we do not want to copy the pointer to the next object. 

What we want to have happen is for the reference member initialization to be left alone on construction and assignment. 

```C++
class TypeInfo { 
// Same As Above
public: // methods
  TypeInfo(const TypeInfo&) {}
  TypeInfo& operator=(const TypeInfo&) { return *this; }
};
```

This leaves a little more work for the programmer if the copy constructor and assignment were not needed. It's up to you if the benefit of simply saying `typeInfo.Name` is more desirable than `typeInfo.Name()`.
