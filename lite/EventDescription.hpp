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

    // Event path name.
    string path;

    // Map to all user properties provided by the event.
    unordered_map<string, FMOD_STUDIO_USER_PROPERTY> userPropertyMap;

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
    {
      // Get the number of user properties.
      int userPropertyCount = 0;
      FmodCall(description->getUserPropertyCount(&userPropertyCount));

      // For each user property:
      for (int i = 0; i < userPropertyCount; ++i)
      {
        // Get a user property by index.
        FMOD_STUDIO_USER_PROPERTY userProperty;
        FmodCall(description->getUserPropertyByIndex(i, &userProperty));

        // Save the user property.
        userPropertyMap[userProperty.name] = userProperty;
      }
    }

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
      auto it = userPropertyMap.find(name);
      if (it == userPropertyMap.end()) return { nullptr, FMOD_STUDIO_USER_PROPERTY_TYPE_FORCEINT };
      return it->second;
    }

    friend ostream& operator<<(ostream& os, const EventDescription& eventDescription)
    {
      return os << eventDescription.path;
    }
  };
} // namespace lite
