#pragma once

#include "EventHandler.hpp"
#include "FmodInclude.hpp"

namespace lite
{
  class EventInstance
  {
  private: // data

    EventDescription* description = nullptr;
    fmod::EventInstance* instance = nullptr;

    EventHandler onAudioShutdown = { "AudioShutdown", this, &EventInstance::OnAudioShutdown };
    EventHandler onAudioUpdate = { "AudioUpdate", this, &EventInstance::OnAudioUpdate };

  public: // data

    EventDescription* Description() const { return description; }

    // 3D attributes
    FMOD_VECTOR Forward   = FMOD_VECTOR{ 0, 0, 1 };
    FMOD_VECTOR Position  = FMOD_VECTOR{ 0, 0, 0 };
    FMOD_VECTOR Up        = FMOD_VECTOR{ 0, 1, 0 };
    FMOD_VECTOR Velocity  = FMOD_VECTOR{ 0, 0, 0 };

  public: // methods

    EventInstance()
    {}

    EventInstance(EventInstance&& b) :
      description(b.description),
      instance(b.instance)
    {
      b.description = nullptr;
      b.instance = nullptr;
    }

    explicit EventInstance(fmod::EventInstance* instance_, EventDescription* description_) :
      description(description_),
      instance(instance_)
    {
    }

    ~EventInstance()
    {
      Release();
    }

    EventInstance& operator=(EventInstance&& b)
    {
      Release();

      description = b.description;
      instance = b.instance;

      b.description = nullptr;
      b.instance = nullptr;

      return *this;
    }

    float GetParameter(const string& name) const
    {
      // Get the instance.
      fmod::ParameterInstance* paramInstance = nullptr;
      FmodCall(instance->getParameter(name.c_str(), &paramInstance), false);

      // Get the value.
      float value = 0;
      FmodCall(paramInstance->getValue(&value), 0);
      return value;
    }

    // Returns whether the playback is playing, idle, 
    //  sustaining, stopped, starting, or stopping. 
    FMOD_STUDIO_PLAYBACK_STATE GetPlaybackState() const
    {
      FMOD_STUDIO_PLAYBACK_STATE state = FMOD_STUDIO_PLAYBACK_STOPPED;
      if (!IsValid()) return state;
      FmodCall(instance->getPlaybackState(&state), state);
      return state;
    }

    // Returns whether this handle is valid.
    bool IsValid() const
    {
      return instance != nullptr;
    }

    // Releases the handle. The sound may continue 
    //  playing if Stop isn't called!
    void Release()
    {
      if (instance)
      {
        FmodCall(instance->release());
        instance = nullptr;
      }
    }

    bool SetParameter(const string& name, float value)
    {
      // Get the instance.
      fmod::ParameterInstance* paramInstance = nullptr;
      FmodCall(instance->getParameter(name.c_str(), &paramInstance), false);

      // Set the value.
      FmodCall(paramInstance->setValue(value), false);
      return true;
    }

    // Starts playing the sound.
    void Start()
    {
      if (!IsValid()) return;
      FmodCall(instance->start());
    }

    // Stops playing the sound (with option to 
    //  allow a gradual fadeout).
    void Stop(bool allowFadeout = false)
    {
      if (!IsValid()) return;
      FmodCall(instance->stop(allowFadeout ? FMOD_STUDIO_STOP_ALLOWFADEOUT : FMOD_STUDIO_STOP_IMMEDIATE));
    }

  private: // methods

    // Releases EventInstance handles in case the user has
    //  any lying around. Good-guy game engine :)
    void OnAudioShutdown(EventData&)
    {
      Release();
    }

    // Updates 3D attributes for this instance.
    void OnAudioUpdate(EventData&)
    {
      FMOD_3D_ATTRIBUTES attributes = { Position, Velocity, Forward, Up };
      FmodCall(instance->set3DAttributes(&attributes));
    }
  };
} // namespace lite