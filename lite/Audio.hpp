#pragma once

#include "Console.hpp"
#include "EventDescription.hpp"
#include "EventInstance.hpp"
#include "FmodInclude.hpp"
#include "ListenerDescription.hpp"
#include "PathInfo.hpp"
#include "SoundBank.hpp" 

namespace lite // types
{
  // Manages the FMOD Studio system and keeps track of 
  //  event descriptions and sound banks.
  class Audio
  {
  private: // data

    // Maximum number of channels allowed on the system.
    static const int maxChannels = 512;

    // Map of event names to their description objects.
    unordered_map<string, EventDescription> eventDescriptionMap;

    // Map of bank names to sound bank objects.
    unordered_map<string, SoundBank> soundBankMap;

    // Fmod Studio's system interface.
    fmod::System* system;

  public: // properties

    // 3D attributes for the listener.
    ListenerDescription Listener;

  public: // methods

    // Initializes FMOD Studio and loads all sound banks.
    Audio() :
      Listener(system)
    {
      // Create the FMOD Studio system.
      FmodCall(fmod::System::create(&system));
      FmodCall(system->initialize(
        maxChannels,               // max channels capable of playing audio
        FMOD_STUDIO_INIT_NORMAL,   // studio-specific flags
        FMOD_INIT_3D_RIGHTHANDED,  // regular flags
        nullptr));                 // extra driver data

      // For each file in the "sounds" directory with a ".bank" extension:
      for (string& file : PathInfo(config::Sounds).FilesWithExtension("bank"))
      {
        // Load the sound bank from file.
        fmod::Bank* bank = nullptr;
        FmodCall(system->loadBankFile(file.c_str(), FMOD_STUDIO_LOAD_BANK_NORMAL, &bank));

        string bankName = PathInfo(file).BaseFilename();
        soundBankMap.emplace(bankName, SoundBank(bank, bankName));
      }

      // For each sound bank:
      for (auto& bankPair : soundBankMap)
      {
        // For each event in the bank:
        for (fmod::EventDescription* eventDescription : bankPair.second.GetEventList())
        {
          // Get the path to the event, e.g. "event:/Ambience/Country"
          char pathBuffer[1024];
          int retrieved = 0;
          FmodCall(eventDescription->getPath(pathBuffer, sizeof(pathBuffer), &retrieved));

          auto path = string(pathBuffer, static_cast<size_t>(retrieved-1));
          eventDescriptionMap.emplace(path, EventDescription(eventDescription, path));
        }
      }

      Note(*this);
    }

    ~Audio()
    {
      // Invoke the AudioShutdown event. This lets the EventListener objects
      //  release so that they don't try to release after the FMOD Studio
      //  system is released below.
      EventData eventData;
      eventData["Audio"] = this;
      InvokeEvent("AudioShutdown", eventData);

      // Destroy the FMOD Studio system.
      FmodCall(system->release());
    }

    // Creates an event instance which can be used for playing the event.
    //  May return an invalid handle; check with InvalidHandle::IsValid.
    EventInstance CreateEventInstance(const string& eventPath)
    {
      EventDescription* description = FindEventDescription(eventPath);
      if (!description) return {};
      return EventInstance(description->CreateInstance(), description);
    }

    // Returns the event description corresponding to an event path.
    //  Returns null if the event could not be found.
    EventDescription* FindEventDescription(const string& eventPath)
    {
      // Find the event description.
      auto it = eventDescriptionMap.find(eventPath);
      if (it == eventDescriptionMap.end()) return nullptr;
      return &it->second;
    }

    // Plays a one-shot event. Returns false if the event cannot
    //  be found or the event is not one-shot.
    bool PlayOneshot(const string& eventPath)
    {
      EventDescription* description = FindEventDescription(eventPath);
      if (!description || !description->IsOneshot()) return false;

      CreateEventInstance(eventPath).Start();
      return true;
    }

    // Formats the sound banks and events available in the audio system.
    friend ostream& operator<<(ostream& os, const Audio& a)
    {
      os << "Audio:\n";
      Format(os, a.eventDescriptionMap) << "\n";
      Format(os, a.soundBankMap);
      return os;
    }

    // Updates the FMOD Studio system.
    void Update()
    {
      // Invoke the AudioUpdate event to allow
      //  EventInstance objects to update.
      EventData eventData;
      eventData["Audio"] = this;
      InvokeEvent("AudioUpdate", eventData);

      // Update the FMOD Studio system.
      FmodCall(system->update());
    }

  private: // methods

    // Formatted output for the event description map.
    static ostream& Format(ostream& os, const unordered_map<string, EventDescription>& eventDescriptionMap)
    {
      os << Tabs(1) << "Event descriptions:";
      for (auto& eventPair : eventDescriptionMap)
      {
        os << "\n" << Tabs(1) << eventPair.second;
      }
      return os;
    }

    // Formatted output for the sound bank map.
    static ostream& Format(ostream& os, const unordered_map<string, SoundBank>& soundBankMap)
    {
      os << Tabs(1) << "Sound banks:";
      for (auto& bankPair : soundBankMap)
      {
        os << "\n" << Tabs(1) << bankPair.second;
      }
      return os;
    }
  };
} // namespace lite