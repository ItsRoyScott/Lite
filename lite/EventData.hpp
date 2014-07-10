#pragma once

#include "Essentials.hpp"
#include <unordered_map>
#include "Variant.hpp"

namespace lite
{
  // Stores any kind of a data with a collection of variants.
  class EventData
  {
  private: // data

    // Name of the event; useful for functions handling multiple events.
    string eventName;

    // Stores all possible data as a single hash-map.
    unordered_map<string, Variant> payload;

    // Friend EventSystem so it may set the eventName field.
    friend class EventSystem;

  public: // methods

    virtual ~EventData() {}

    // Returns whether the named data exists.
    bool Exists(const string& name) const
    {
      return payload.find(name) != payload.end();
    }

    // Returns the data associated by name and type 'T'.
    //  The types must exactly match.
    template <class T>
    T& Get(const string& name)
    {
      Variant& variant = payload[name];

      FatalIf(!variant.IsValid(), "EventData::Get called with invalid name");

      // Assign a new type if the given type 'T' 
      //  doesn't match the variant's type.
      if (!variant.IsType<T>())
      {
        variant.Assign<T>();
      }

      return variant.Ref<T>();
    }

    // Returns the name of the event being called.
    const string& GetEventName() const
    {
      return eventName;
    }

    // Accesses the named data.
    Variant& operator[](const string& name)
    {
      return payload[name];
    }
  };
} // namespace lite