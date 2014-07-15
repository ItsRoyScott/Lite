#pragma once

#include "Essentials.hpp"
#include "WindowsInclude.h"

namespace lite
{
  // Wraps the Win32 console for printing in color.
  class Console : public Singleton<Console>
  {
  private: // data

    // Buffers output until newline.
    string buffer;

  public: // types

    // Color of the text being written to the console.
    enum Color
    {
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

  public: // methods

    // Prints a string to the console.
    void Print(const string& str)
    {
      buffer += str;

      // Flush if the last write ended with a newline or carriage-return.
      if (buffer.size() && (buffer.back() == '\n' || buffer.back() == '\r'))
      {
        buffer.pop_back(); // puts appends newline
        puts(buffer.c_str());
        buffer.clear();
      }
    }

    // Set the text color for writing to the console.
    void SetTextColor(Color c = BrightWhite)
    {
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), WORD(c));
    }

    // Change the text color.
    Console& operator<<(Color c)
    {
      SetTextColor(c);
      return *this;
    }

    // Writes a string to the console.
    Console& operator<<(const string& str)
    {
      Print(str);
      return *this;
    }

    // Writes any object to the console that supports stringstream insertion.
    template <class T>
    Console& operator<<(const T& object)
    {
      Print(ToString(object));
      return *this;
    }

  private: // methods

    Console()
    {
      // Create the console window.
      AllocConsole();
      MoveWindow(GetConsoleWindow(), 0, 0, 640, 850, TRUE);

      // Redirect stdout and stderr to the new console.
      FILE* file;
      freopen_s(&file, "CONOUT$", "w", stdout);
      freopen_s(&file, "CONOUT$", "w", stderr);
    }

    ~Console()
    {
      // Destroy the console window.
      FreeConsole();
    }

    friend Singleton < Console > ;
  };
} // namespace lite

// All log functions allow ostream-like insertion for formatting the string.

// Log fatal errors in bright red font.
#define Fatal(...)                 DEBUG_ONLY(LogPrint(BrightRed, __VA_ARGS__); BREAKPOINT;)
#define FatalIf(condition, ...)    DEBUG_ONLY(DO_IF(condition, Fatal(__VA_ARGS__)))

// Print to the log in a specified color (see Console).
#define LogPrint(color, ...)       DEBUG_ONLY(SCOPE(lite::Console::Instance() << lite::Console::color << __VA_ARGS__ << "\n"))

// Log notes in white font.
#define Note(...)                  DEBUG_ONLY(LogPrint(White, __VA_ARGS__))
#define NoteIf(condition, ...)     DEBUG_ONLY(DO_IF(condition, Note(__VA_ARGS__)))

// Log warnings in yellow font.
#define Warn(...)                  DEBUG_ONLY(static int _count = 0; if (++_count <= 3) { LogPrint(BrightYellow, __VA_ARGS__); } )
#define WarnIf(condition, ...)     DEBUG_ONLY(DO_IF(condition, Warn(__VA_ARGS__)))