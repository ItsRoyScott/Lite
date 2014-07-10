#pragma once

#include "EventSystem.hpp"

namespace lite
{
  // Registers a handler with the global EventSystem upon 
  //  construction and removes it upon destruction.
  class EventHandler
  {
  private: // data

    // Name of the event being listened for.
    string eventName;

    // Unique id of this handler.
    size_t id = (size_t) -1;

  public: // methods

    // Cannot copy or move event handler because a previously 
    //  captured 'this' pointer may be pointing to an invalid
    //  object after the copy.
    EventHandler() = delete;
    EventHandler(const EventHandler&) = delete;
    EventHandler(EventHandler&&) = delete;

    // Constructs the handler from an event name and function object.
    EventHandler(string eventName_, function<void(EventData&)> fn) :
      eventName(move(eventName_)),
      id(EventSystem::GenerateHandlerId())
    {
      EventSystem::Instance().AddHandler(eventName, move(fn), id);
    }

    // Constructs the handler from an event name, a 'this' pointer, and a pointer to the
    //  member function handling the event.
    template <class ThisType, class MemberFunctionPointer>
    EventHandler(string eventName_, ThisType* this_, MemberFunctionPointer memfn) :
      EventHandler(move(eventName_), [=](EventData& data) { (this_->*memfn)(data); })
    {
    }

    // Unregisters the event handler from the global event system.
    ~EventHandler()
    {
      Clear();
    }

    // Cannot copy or move event handler because a previously 
    //  captured 'this' pointer may be pointing to an invalid
    //  object after the copy.
    EventHandler& operator=(const EventHandler&) = delete;
    EventHandler& operator=(EventHandler&&) = delete;

    // Unregisters the event handler from the global event system.
    void Clear()
    {
      EventSystem::Instance().RemoveHandler(eventName, id);
    }
  };
} // namespace lite
