#pragma once

#include "ComponentForward.hpp"
#include "Essentials.hpp"
#include "Reflection.hpp"

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
    virtual const TypeInfo& GetType() const = 0;

    // Sets whether the component is active and updating.
    virtual void SetActive(bool active) = 0;

  protected: // methods

    // Called on SetActive(true).
    virtual void Activate() = 0;

    // Called on SetActive(false).
    virtual void Deactivate() = 0;

    // Initializes the component after all child objects
    //  have been initialized and other components added
    //  to the object. Or, at runtime it is called immediately
    //  when one makes a call to GameObject::AddComponent.
    virtual void Initialize() = 0;

    // Pulls updates from systems into components to prepare for game logic.
    virtual void PullFromSystems() = 0;

    // Pushes updates from game logic into systems to prepare for system update.
    virtual void PushToSystems() = 0;

    // Called by the game object when a new owner is set for the component.
    virtual void SetOwner(GameObject& owner) = 0;

    // Updates the component.
    virtual void Update() = 0;

    // The GameObject can access various callbacks on components.
    friend class GameObject;
  };

  // Base class to simplify programming components.
  template <class T>
  class Component : public IComponent
  {
  private: // data
    
    bool              isActive = true;
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

    Component() = default;
    Component(const Component& b) {}
    ~Component() override {}
    Component& operator=(const Component&) = delete;

    // Creates a copy of this component.
    unique_ptr<IComponent> Clone() const override
    {
      // Make a new component of type T.
      return make_unique<T>(*static_cast<const T*>(this));
    }

    // Returns the type info for this component (using typeid).
    const TypeInfo& GetType() const override
    {
      return TypeOf<T>();
    }

    // Sets whether the component is active and updating.
    void SetActive(bool active) override
    {
      if (isActive)
      {
        if (!active)
        {
          Deactivate();
        }
      }
      else // !isActive
      {
        if (active)
        {
          Activate();
        }
      }

      isActive = active;
    }

    // Default ostream formatting: prints the type of the component.
    friend ostream& operator<<(ostream& os, const Component<T>& c)
    {
      return os << c.GetType().Name;
    }

    // Default istream reading: does nothing.
    friend istream& operator>>(istream& is, Component<T>&)
    {
      return is;
    }

  protected: // methods

    // Called on SetActive(true).
    void Activate() override {}

    // Called on SetActive(false).
    void Deactivate() override {}

    // Initializes the component after all child objects
    //  have been initialized and other components added
    //  to the object. Or, at runtime it is called immediately
    //  when one makes a call to GameObject::AddComponent.
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
} // namespace lite