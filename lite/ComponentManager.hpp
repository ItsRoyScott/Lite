#pragma once

#include "Component.hpp"
#include <unordered_map>

namespace lite
{
  // Maintains a map of creation functions for all component types.
  class ComponentManager : public Singleton < ComponentManager >
  {
  private: // data

    unordered_map<string, function<unique_ptr<IComponent>()>> components;

  public: // methods

    // Creates a component by name.
    unique_ptr<IComponent> Create(const string& name)
    {
      auto it = components.find(name);
      if (it == components.end())
      {
        Fatal("Failed to find create function for component " << name << "\nCurrently registered components:\n" << *this);
        return nullptr;
      }
      return it->second();
    }

    // Registers a component with the manager.
    template <class T>
    void Register(string name = TypeOf<T>().Name)
    {
      // Make a generic 'create' function returning an IComponent pointer.
      auto create = []() -> unique_ptr < IComponent >
      {
        return make_unique<T>();
      };

      components.emplace(move(name), move(create));
    }

    // Prints all components registered to the manager.
    friend ostream& operator<<(ostream& os, const ComponentManager& cm)
    {
      for (auto& createPair : cm.components)
      {
        os << createPair.first << "\n";
      }
      return os;
    }
  };

  // Registers a component with the ComponentManager instance.
  template <class T>
  void RegisterComponent(string name = TypeOf<T>().Name)
  {
    return ComponentManager::Instance().Register<T>(move(name));
  }
} // namespace lite