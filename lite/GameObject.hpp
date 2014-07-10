#pragma once

#include "Component.hpp"
#include "Essentials.hpp"
#include "EventHandler.hpp"
#include "Metadata.hpp"
#include "Transform.hpp"

namespace lite
{
  class GameObject
  {
  private: // data

    vector<unique_ptr<GameObject>> children;
    vector<unique_ptr<Component>> components;
    bool  destroyFlag = false;
    uint64_t identifier = GenerateIdentifier();
    bool initializeFlag = false;
    string name;
    GameObject* parent = nullptr;
    vector<GameObject*> toDestroy;
    vector<GameObject*> toInitialize;

  public: // properties

    // Children game objects attached to this game object.
    const vector<unique_ptr<GameObject>>& Children() const { return children; }

    // Whether this object will be destroyed at the end of the frame.
    const bool& DestroyFlag() const { return destroyFlag; }

    // Unique identifer of this game object.
    const uint64_t& Identifier() const { return identifier; }

    // Name of this game object. (May be empty)
    const string& Name() const { return name; }

    // Pointer to the parent game object. (May be null)
    GameObject* const& Parent() const { return parent; }

  public: // methods

    explicit GameObject(
      vector<unique_ptr<Component>> components_ = vector<unique_ptr<Component>>(),
      GameObject* parent_ = nullptr,
      vector<unique_ptr<GameObject>> children_ = vector<unique_ptr<GameObject>>()) :
        components(move(components_)),
        parent(parent_),
        children(move(children_))
    {
    }

    virtual ~GameObject() {}

    // Adds a new child object by prefab.
    GameObject& AddChild(const GameObject& prefab)
    {
      children.push_back(make_unique<GameObject>(prefab));
      return *children.back();
    }

    // Destroys this object along with all of its children.
    void Destroy()
    {
      destroyFlag = true;

      // Destroy each child.
      for (auto& child : children)
      {
        child->Destroy();
      }

      // Queue this object to parent's toDestroy list.
      parent->toDestroy.push_back(this);
    }

    // Finds a game object given a predicate condition. (May return null)
    //  Signature of the predicate is bool(GameObject&).
    template <class Predicate>
    GameObject* FindChild(Predicate pred)
    {
      for (auto& child : children)
      {
        if (pred(*child))
        {
          return child.get();
        }
      }
      return nullptr;
    }

    // Finds a component given a predicate condition. (May return null)
    //  Signature of the predicate is bool(IComponent&).
    template <class Predicate>
    IComponent* FindComponent(Predicate pred)
    {
      for (auto& component : components)
      {
        if (pred(*component))
        {
          return component.get();
        }
      }
      return nullptr;
    }

    // Finds a child by its name. (May return null)
    GameObject* GetChild(const string& name)
    {
      return FindChild([&](GameObject& object) { return object.Name == name; });
    }

    // Finds a child by identifier. (May return null)
    GameObject* GetChild(uint64_t id)
    {
      return FindChild([&](GameObject& object) { return object.Identifier == id; });
    }

    // Returns the index of a child object. (May return numeric_limits<size_t>::max)
    size_t GetChildIndex(GameObject* object)
    {
      for (size_t i = 0; i < children.size(); ++i)
      {
        if (children[i].get() == object)
        {
          return i;
        }
      }
      return size_t(-1);
    }

    // Returns a component by a type. (May return null)
    template <class T>
    T* GetComponent()
    {
      return FindComponent([](IComponent& component) { return component.GetType() == typeid(T); });
    }

    // Returns a component by its type name. (May return null)
    IComponent* GetComponent(const string& typeName)
    {
      return FindComponent([&](IComponent& component) { return component.GetTypeName() == typeName; });
    }

    // Initializes child objects, then components of this object.
    void Initialize()
    {
      // Initialize all child objects.
      for (auto& child : children)
      {
        child->Initialize();
      }

      // Then initialize all components.
      for (auto& component : components)
      {
        component->Initialize();
      }
    }

    // Changes the name of this object.
    void SetName(string name_)
    {
      name = move(name_);
    }

    // Updates child objects, then the components of this object.
    void Update()
    {
      // Remember the number of objects requested for destruction last frame.
      size_t objectsToDestroy = toDestroy.size();

      // Update children objects.
      for (auto& child : children)
      {
        child->Update();
      }

      // Update components.
      for (auto& component : components)
      {
        component->Update();
      }

      // Destroy all objects queued for destruction last frame.
      toDestroy.erase(toDestroy.begin(), toDestroy.begin() + objectsToDestroy);
    }

  private: // methods

    // Adds a child object directly into the current list of objects.
    void AddChild(unique_ptr<GameObject> object)
    {
      children.push_back(move(object));
    }

    // Adds a component directly into the current list of components.
    void AddComponent(unique_ptr<Component> component)
    {
      components.push_back(move(component));
    }

    // Generates a new unique identifier for the game object.
    static uint64_t GenerateIdentifier()
    {
      static uint64_t id = 0;
      return id++;
    }
  };

} // namespace lite