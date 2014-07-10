#pragma once

#include "FmodInclude.hpp"

namespace lite
{
  class ListenerDescription
  {
  private: // data

    fmod::System** system;

  public: // data

    // 3D attributes
    FMOD_VECTOR Forward   = FMOD_VECTOR{ 0, 0, -1 };
    FMOD_VECTOR Position  = FMOD_VECTOR{ 0, 0, 0 };
    FMOD_VECTOR Up        = FMOD_VECTOR{ 0, 1, 0 };
    FMOD_VECTOR Velocity  = FMOD_VECTOR{ 0, 0, 0 };

  public: // methods

    ListenerDescription(fmod::System*& system_) :
      system(&system_)
    {
    }

    // Updates the system with the 3D attributes.
    void Update()
    {
      FMOD_3D_ATTRIBUTES attributes = { Position, Velocity, Forward, Up };
      FmodCall((*system)->setListenerAttributes(&attributes));
    }
  };
} // namespace lite