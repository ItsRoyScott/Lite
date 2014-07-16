#pragma once

#include "Console.hpp"
#include "WindowsInclude.h"

#include "FMOD/fmod_errors.h"
#include "FMOD/fmod_studio.hpp"

#pragma comment(lib, "FMOD/fmodstudio_vc.lib")

namespace lite
{
  namespace fmod = FMOD::Studio;
} // namepace lite

// Wraps all FMOD calls which return an FMOD_RESULT.
//  Prints a warning to the console when something bad happens.
//  Returns the second parameter out of the function.
#define FmodCall(x, ...) SCOPE( \
  FMOD_RESULT fmodResult;  \
  if ((fmodResult = (x)) != FMOD_OK) \
  { \
    Warn("FMOD error: (" << int(fmodResult) << ") " << #x << "\n" << FMOD_ErrorString(fmodResult)); \
    return __VA_ARGS__; \
  })
