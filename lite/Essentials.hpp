#pragma once

#include <algorithm>    // find_if, etc.
#include <array>        // array
#include <cmath>        // pow, etc.
#include <cstdint>      // uint32_t
#include <functional>   // function
#include <iosfwd>       // istream, ostream, etc. forward declarations
#include <limits>       // numeric_limits
#include <memory>       // unique_ptr
#include <sstream>      // stringstream
#include <string>       // string
#include <type_traits>  // decay_t, conditional_t, etc.
#include <typeindex>    // type_index
#include <utility>      // forward, move
#include <vector>       // vector

namespace lite // types
{
  using namespace std;

  // A lightweight singleton which uses the most recently
  //  created instance as the singleton.
  template <class T>
  struct LightSingleton
  {
    LightSingleton()
    {
      CurrentInstance() = static_cast<T*>(this);
    }

    ~LightSingleton()
    {
      CurrentInstance() = nullptr;
    }

    // Returns a pointer to the most recently created instance.
    static T*& CurrentInstance()
    {
      static T* instance = nullptr;
      return instance;
    }
  };

  // Implements the singleton design pattern.
  template <class T>
  struct Singleton
  {
    static T& Instance()
    {
      static T instance;
      return instance;
    }
  };
} // namespace lite

namespace lite // data
{
  // Whether debug mode is enabled.
  static const bool DebugMode =
#if defined(_DEBUG) && !defined(RELEASE_MODE_DEBUGGING)
    true;
#else
    false;
#endif

  // Whether release mode is enabled.
  static const bool ReleaseMode = !DebugMode;
}

#include "Config.hpp" // paths to assets

namespace lite // functions
{
  // Aligns an object to a buffer given the desired alignment,
  //  a pointer to the buffer, and amount of space in the buffer.
  template <class T>
  inline T* align(size_t alignment, void* p)
  {
    size_t offset = (size_t) ((uintptr_t) p & (alignment - 1));
    return (T*)((char*)p + (0 < offset ? alignment - offset : offset));
  }

  static const unsigned tabSize = 2;

  // Returns a tab count as a string.
  inline string Tabs(unsigned count)
  {
    return count ? string(count*tabSize, ' ') : string();
  }

  // Converts an object to its string representation.
  //  Check out boost::lexical_cast, it's way cooler.
  template <class T>
  string ToString(const T& object, unsigned tabs = 0)
  {
    stringstream ss;
    ss << Tabs(tabs) << object;
    return ss.str();
  }

  // Converts a multi-byte string to a wide string.
  inline wstring MultibyteToWideChar(const string& input)
  {
    // Convert to multibyte using a character buffer.
    size_t size;
    auto buffer = vector<wchar_t>(input.size() + 1);
    mbstowcs_s(&size, buffer.data(), input.size() + 1, input.c_str(), input.size());
    return wstring(buffer.data(), size);
  }

  // Converts a wide string to multi-byte string.
  inline string WideCharToMultibyte(const wstring& input)
  {
    size_t size;
    auto buffer = vector<char>(input.size() + 1);
    wcstombs_s(&size, buffer.data(), input.size() + 1, input.c_str(), input.size());
    return string(buffer.data(), size);
  }
} // namespace lite

// A breakpoint you can write in code.
#define BREAKPOINT SCOPE(__debugbreak())

// Enable macros/code only in debug mode.
#if defined(_DEBUG) && !defined(RELEASE_MODE_DEBUGGING)
  #define DEBUG_ONLY(x) SCOPE(x)
#else
  #define DEBUG_ONLY(x)
#endif

// Macro-ized if condition.
#define DO_IF(condition, ...) SCOPE( if (condition) { __VA_ARGS__; } )

// Forms a scope that won't mess with calling code.
#define SCOPE(...) do { __VA_ARGS__; } while (0);

#pragma warning(disable: 4100) // unreferenced formal parameter
#pragma warning(disable: 4127) // conditional expression is constant
#pragma warning(disable: 4505) // unreferenced local function has been removed
#ifndef _DEBUG
#pragma warning(disable: 4189) // local variable is initialized but not referenced
#endif