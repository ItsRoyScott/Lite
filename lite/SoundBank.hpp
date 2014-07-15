#pragma once

#include "Essentials.hpp"
#include "FmodInclude.hpp"

namespace lite
{
  class SoundBank
  {
  private: // data

    fmod::Bank* bank;
    string      name;

  public: // properties

    const string& Name() const { return name; }

  public: // methods

    SoundBank(SoundBank&& b) :
      bank(b.bank),
      name(move(b.name))
    {
      b.bank = nullptr;
    }

    SoundBank(fmod::Bank* bank_, string name_) :
      bank(bank_),
      name(move(name_))
    {
    }

    ~SoundBank()
    {
      Clear();
    }

    SoundBank& operator=(SoundBank&& b)
    {
      Clear();
      bank = b.bank;
      name = move(b.name);
      b.bank = nullptr;
      return *this;
    }

    // Unloads the bank.
    void Clear()
    {
      if (bank)
      {
        bank->unload();
        bank = nullptr;
      }
    }

    // Returns array of FMOD event descriptions stored in this bank.
    vector<fmod::EventDescription*> GetEventList() const
    {
      FatalIf(bank == nullptr, "GetEventList: Bank is null");

      // Get an array of EventDescription*'s from the bank.
      int eventCount = 0;
      FmodCall(bank->getEventCount(&eventCount), {});
      if (eventCount == 0) return {};

      auto result = vector<fmod::EventDescription*>(static_cast<size_t>(eventCount), nullptr);
      FmodCall(bank->getEventList(result.data(), result.size(), &eventCount), {});

      // Verify that all expected events have been filled in.
      if (static_cast<size_t>(eventCount) != result.size())
      {
        Warn("Unable to read all events from sound bank " + name);
        result.resize(static_cast<size_t>(eventCount));
      }

      return move(result);
    }

    friend ostream& operator<<(ostream& os, const SoundBank& bank)
    {
      return os << bank.name;
    }
  };
} // namespace lite