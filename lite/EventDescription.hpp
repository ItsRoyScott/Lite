#pragma once

#include "Essentials.hpp"
#include "FmodInclude.hpp"

namespace lite
{
  class EventDescription
  {
  private: // data

    fmod::EventDescription* description;
    bool isOneshot;
    string path;

  public: // properties

    // Whether the sound will naturally terminate and
    //  can be used as a fire-and-forget sort of sound.
    bool IsOneshot() const { return isOneshot; }

    // The event's path name.
    const string& Path() const { return path; }

  public: // methods

    EventDescription(fmod::EventDescription* description_, string path_) :
      description(description_),
      path(move(path_))
    {
      FmodCall(description->isOneshot(&isOneshot));
    }

    fmod::EventInstance* CreateInstance()
    {
      fmod::EventInstance* instance = nullptr;
      FmodCall(description->createInstance(&instance), nullptr);
      return instance;
    }

    friend ostream& operator<<(ostream& os, const EventDescription& eventDescription)
    {
      return os << eventDescription.path;
    }
  };
} // namespace lite
