#pragma once

#include "EventDescription.hpp"
#include "EventInstance.hpp"
#include "FmodInclude.hpp"
#include "ListenerDescription.hpp"
#include "Logging.hpp"
#include "PathInfo.hpp"
#include "SoundBank.hpp" 

namespace lite // declarations
{
  // ToString conversion for the sound bank map.
  string ToString(const unordered_map<string, SoundBank>& soundBankMap, unsigned tabs = 0);

  // ToString conversion for the event description map.
  string ToString(const unordered_map<string, EventDescription>& eventDescriptionMap, unsigned tabs = 0);
} // namespace lite

namespace lite // types
{
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
      // Invoke the AudioShutdown event.
      EventData eventData;
      eventData["Audio"] = this;
      InvokeEvent("AudioShutdown", eventData);

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

    // ToString conversion for the Audio system.
    friend string ToString(const Audio& audio, unsigned tabs = 0)
    {
      string str = Tabs(tabs) + "Audio:\n";
      str += ToString(audio.eventDescriptionMap, tabs + 1) + "\n";
      str += ToString(audio.soundBankMap, tabs + 1);
      return move(str);
    }

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

  };
} // namespace lite

namespace lite // functions
{
  // ToString conversion for the event description map.
  inline string ToString(const unordered_map<string, EventDescription>& eventDescriptionMap, unsigned tabs)
  {
    string str = Tabs(tabs) + "Event descriptions:\n";
    for (auto& eventPair : eventDescriptionMap)
      str += Tabs(tabs + 1) + ToString(eventPair.second) + "\n";
    if (str.back() == '\n') str.pop_back();
    return move(str);
  }

  // ToString conversion for the sound bank map.
  inline string ToString(const unordered_map<string, SoundBank>& soundBankMap, unsigned tabs)
  {
    string str = Tabs(tabs) + "Sound banks:\n";
    for (auto& bankPair : soundBankMap)
      str += Tabs(tabs + 1) + ToString(bankPair.second) + "\n";
    if (str.back() == '\n') str.pop_back();
    return move(str);
  }
} // namespace lite