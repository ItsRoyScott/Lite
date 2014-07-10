#pragma once

#include "EventData.hpp"
#include "Logging.hpp"

namespace lite // types
{
  // Function capable of handling events.
  typedef function<void(EventData&)> EventHandlerFunction;

  // Generic system for registering event handlers and invoking events.
  class EventSystem : public Singleton<EventSystem>
  {
  private: // data

    typedef pair<size_t, EventHandlerFunction> IdHandlerPair;

    // Stores all event handlers mapped by name.
    unordered_map<string, vector<IdHandlerPair>> eventHandlerMap;

    friend class EventHandler;

  public: // methods

    // Adds a handler given its event name, handler function, and an optional unique id.
    void AddHandler(const string& eventName, EventHandlerFunction fn, size_t id = size_t(-1))
    {
      if (id == size_t(-1))
      {
        id = GenerateHandlerId();
      }
      eventHandlerMap[eventName].push_back(make_pair(id, move(fn)));
    }

    // Returns whether the event exists in the system.
    bool Exists(const string& name) const
    {
      return eventHandlerMap.find(name) != eventHandlerMap.end();
    }

    // Returns a unique id for event handlers.
    static size_t GenerateHandlerId()
    {
      static size_t id = 0;
      return id++;
    }

    // Call an event with no data.
    size_t Invoke(string name)
    {
      EventData data;
      return Invoke(move(name), data);
    }

    // Call an event with data.
    size_t Invoke(string name, EventData& data)
    {
      if (!Exists(name)) return 0;

      // Get the map of event handlers, /then/ move 
      //  'name' into the EventData structure.
      auto& handlers = eventHandlerMap[name];
      data.eventName = move(name);

      // Call all handlers registered for this event.
      //  Called in reverse order so on-destruction events will cleanup
      //  in the correct order.
      for (auto it = handlers.rbegin(); it != handlers.rend(); ++it)
      {
        it->second(data);
      }

      return handlers.size();
    }

    // Removes an event handler given the event name and id.
    //  Note this automatically happens upon ~EventHandler().
    void RemoveHandler(const string& eventName, size_t id)
    {
      vector<IdHandlerPair>& handlerPairs = eventHandlerMap[eventName];

      // Find the handler matching the given id.
      auto it = find_if(
        handlerPairs.begin(),
        handlerPairs.end(),
        [&](IdHandlerPair& pair)
        {
          return pair.first == id;
        });

      // Erase the handler from the array.
      if (it == handlerPairs.end()) return;
      handlerPairs.erase(it);
    }
  };

} // namespace lite

namespace lite // functions
{
  // Call an event with no data.
  inline size_t InvokeEvent(string name)
  {
    return EventSystem::Instance().Invoke(move(name));
  }

  // Call an event with data.
  inline size_t InvokeEvent(string name, EventData& data)
  {
    return EventSystem::Instance().Invoke(move(name), data);
  }
} // namespace lite