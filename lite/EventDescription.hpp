#pragma once

#include "Essentials.hpp"
#include "FmodInclude.hpp"

namespace lite
{
  class EventDescription
  {
  private: // data

    // Pointer to FMOD's description object.
    fmod::EventDescription* description;

    // User property returned when the requested property isn't found.
    FMOD_STUDIO_USER_PROPERTY nullUserProperty = FMOD_STUDIO_USER_PROPERTY{ nullptr, FMOD_STUDIO_USER_PROPERTY_TYPE_FORCEINT };

    // Event path name.
    string path;

  public: // properties

    // Whether the sound will naturally terminate and
    //  can be used as a fire-and-forget sort of sound.
    bool IsOneshot() const
    {
      bool b;
      FmodCall(description->isOneshot(&b), false);
      return b;
    }

    // The event's path name.
    const string& Path() const { return path; }

  public: // methods

    EventDescription(fmod::EventDescription* description_, string path_) :
      description(description_),
      path(move(path_))
    {}

    fmod::EventInstance* CreateInstance()
    {
      fmod::EventInstance* instance = nullptr;
      FmodCall(description->createInstance(&instance), nullptr);
      return instance;
    }

    // Returns a user property by name. The 'name' field will be null if the
    //  property wasn't found.
    FMOD_STUDIO_USER_PROPERTY GetUserProperty(const string& name) const
    {
      FMOD_STUDIO_USER_PROPERTY userProperty = nullUserProperty;
      description->getUserProperty(name.c_str(), &userProperty);
      return userProperty;
    }

    friend ostream& operator<<(ostream& os, const EventDescription& eventDescription)
    {
      return os << eventDescription.path;
    }
  };
} // namespace lite
