#pragma once

#include "Console.hpp"
#include "Essentials.hpp"

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