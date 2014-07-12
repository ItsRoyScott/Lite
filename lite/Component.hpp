#pragma once

#include "ComponentForward.hpp"
#include "Essentials.hpp"

namespace lite
{
  class GameObject;

  // Interface all components must inherit from.
  class IComponent
  {
  public: // methods

    virtual ~IComponent() {}

    // Creates a copy of this component.
    virtual unique_ptr<IComponent> Clone() const = 0;

    // Returns the type info for this component (using typeid).
    virtual const type_info& GetType() const = 0;

    // Returns the hash code of the component type name.
    virtual const size_t& GetTypeHash() const = 0;

    // Returns the name of this component.
    virtual const string& GetTypeName() const = 0;

    // Sets whether the component is active and updating.
    virtual void SetActive(bool active) = 0;

  protected: // methods

    // Called on SetActive(true).
    virtual void Activate() = 0;

    // Called on SetActive(false).
    virtual void Deactivate() = 0;

    // Initializes the component after child objects have been
    //  initialized. No guarantees are made about initialization
    //  order between different objects in the world.
    virtual void Initialize() = 0;

    // Pulls updates from systems into components to prepare for game logic.
    virtual void PullFromSystems() = 0;

    // Pushes updates from game logic into systems to prepare for system update.
    virtual void PushToSystems() = 0;

    // Called by the game object when a new owner is set for the component.
    virtual void SetOwner(GameObject& owner) = 0;

    // Updates the component.
    virtual void Update() = 0;

    friend GameObject;
  };

  template <class T>
  class Component : public IComponent
  {
  private: // data
    
    bool isActive = true;
    lite::GameObject* owner = nullptr;

  public: // properties

    // Whether the component is currently active and updating.
    const bool& IsActive() const { return isActive; }

    // Returns a pointer to the owning game object.
    lite::GameObject* const& Owner() const { return owner; }

    // Recommended way to access an owner; this way the assertion
    //  macro will catch you dereferencing null before you do.
    lite::GameObject& OwnerReference() const 
    {
      FatalIf(!owner, "Dereferencing a null owner pointer");
      return *owner; 
    }

  public: // methods

    ~Component() override {}

    // Creates a copy of this component.
    unique_ptr<IComponent> Clone() const override
    {
      return make_unique<T>(*static_cast<const T*>(this));
    }

    // Returns the type info for this component (using typeid).
    const type_info& GetType() const override
    {
      return typeid(T);
    }

    // Returns the hash code of the component type name.
    const size_t& GetTypeHash() const override
    {
      static size_t hash = GetType().hash_code();
      return hash;
    }

    // Returns the name of this component.
    const string& GetTypeName() const override
    {
      static string name = GetType().name();
      return name;
    }

    // Sets whether the component is active and updating.
    void SetActive(bool active) override
    {
      if (isActive)
      {
        if (!active)
        {
          Activate();
        }
      }
      else // !isActive
      {
        if (active)
        {
          Deactivate();
        }
      }

      isActive = active;
    }

  protected: // methods

    // Called on SetActive(true).
    void Activate() override {}

    // Called on SetActive(false).
    void Deactivate() override {}

    // Initializes the component after child objects have been
    //  initialized. No guarantees are made about initialization
    //  order between different objects in the world.
    void Initialize() override {}

    // Pulls updates from systems into components to prepare for game logic.
    void PullFromSystems() override {}

    // Pushes updates from game logic into systems to prepare for system update.
    void PushToSystems() override {}

    // Called by the game object when a new owner is set for the component.
    void SetOwner(GameObject& owner) override
    {
      this->owner = &owner;
    }

    // Updates the component.
    void Update() override {}
  };

  class ComponentManager : public Singleton<ComponentManager>
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
        Warn("Failed to find create component " << name);
        return nullptr;
      }
      return it->second();
    }

    // Registers a component with the manager.
    template <class T>
    void Register(string name = typeid(T).name())
    {
      // Make a generic 'create' function returning an IComponent pointer.
      auto create = []() -> unique_ptr < IComponent > { return make_unique<T>(); };

      components.emplace(move(name), move(create));
    }
  };

  template <class T>
  void RegisterComponent(string name = typeid(T).name())
  {
    return ComponentManager::Instance().Register<T>(move(name));
  }
} // namespace lite