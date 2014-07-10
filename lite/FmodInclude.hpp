#pragma once

#include "Logging.hpp"
#include "WindowsInclude.h"

#include "FMOD/fmod_errors.h"
#include "FMOD/fmod_studio.hpp"

#pragma comment(lib, "FMOD/fmodstudio_vc.lib")

namespace lite
{
  namespace fmod = FMOD::Studio;

  inline FMOD_RESULT& FmodResult()
  {
    static FMOD_RESULT result;
    return result;
  }
} // namepace lite

// Wraps all FMOD calls which return an FMOD_RESULT.
//  Prints a warning to the console when something bad happens.
//  Returns the second parameter out of the function.
#define FmodCall(x, ...) SCOPE( \
  if ((FmodResult() = (x)) != FMOD_OK) \
  { \
    Warn("FMOD error: (" << int(FmodResult()) << ") " << #x << "\n" << FMOD_ErrorString(FmodResult())); \
    return __VA_ARGS__; \
  })
