#pragma once

#include "Component.hpp"
#include "Essentials.hpp"
#include "EventHandler.hpp"
#include "Transform.hpp"

namespace lite
{
  class GameObject
  {
  private: // data

    vector<unique_ptr<GameObject>> children;
    vector<unique_ptr<IComponent>> components;
    bool  destroyFlag = false;
    uint64_t identifier = GenerateIdentifier();
    string name;
    GameObject* parent = nullptr;
    vector<GameObject*> toDestroy;

  public: // properties

    // Children game objects attached to this game object.
    const vector<unique_ptr<GameObject>>& Children = children;

    // Whether this object will be destroyed at the end of the frame.
    const bool& DestroyFlag = destroyFlag;

    // Unique identifer of this game object.
    const uint64_t& Identifier =  identifier;

    // Name of this game object. (May be empty)
    const string& Name() const { return name; }
    void Name(string name_) { name = move(name_); }

    // Pointer to the parent game object. (May be null)
    GameObject* const& Parent = parent;

  public: // methods

    GameObject() = default;

    GameObject(GameObject&& b) :
      children(move(b.children)),
      components(move(b.components)),
      destroyFlag(b.destroyFlag),
      identifier(b.identifier),
      name(move(b.name)),
      parent(b.parent),
      toDestroy(move(b.toDestroy))
    {}

    GameObject(const GameObject& b) :
      name(b.name)
    {
      CopyChildren(b.children);
      CopyComponents(b.components);
    }

    virtual ~GameObject() 
    {
      Clear();
    }

    GameObject& operator=(GameObject&& b)
    {
      children = move(b.children);
      components = move(b.components);
      destroyFlag = b.destroyFlag;
      identifier = b.identifier;
      name = move(b.name);
      parent = b.parent;
      toDestroy = move(b.toDestroy);

      return *this;
    }

    GameObject& operator=(const GameObject& b)
    {
      Clear();

      name = b.name;
      CopyChildren(b.children);
      CopyComponents(b.components);

      return *this;
    }

    // Adds a new child object by prefab.
    GameObject& AddChild(const GameObject& prefab, bool initialize = true)
    {
      GameObject& object = StoreChild(make_unique<GameObject>(prefab));
      if (initialize)
      {
        object.Initialize();
      }
      return object;
    }

    // Adds a new component by type.
    template <class T>
    T& AddComponent(bool initialize = true)
    {
      return static_cast<T&>(AddComponent(typeid(T).name(), initialize));
    }

    // Adds a new component by name.
    IComponent& AddComponent(const string& name, bool initialize = true)
    {
      // Call on the component manager to create the component.
      IComponent& component = StoreComponent(ComponentManager::Instance().Create(name));
      if (initialize)
      {
        component.Initialize();
      }
      return component;
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
      return FindChild([&](GameObject& object) { return object.Name() == name; });
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
      IComponent* component =  FindComponent([](IComponent& component) { return component.GetType() == typeid(T); });
      return static_cast<T*>(component);
    }

    // Returns a component by its type name. (May return null)
    IComponent* GetComponent(const string& typeName)
    {
      return FindComponent([&](IComponent& component) { return component.GetTypeName() == typeName; });
    }

    // Initializes components, then child objects.
    //  This is meant to be called after a level load so
    //  component initialization can depend on another.
    void Initialize()
    {
      // Then initialize all components.
      for (auto& component : components)
      {
        component->Initialize();
      }

      // Initialize all child objects.
      for (auto& child : children)
      {
        child->Initialize();
      }
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

    // Easy component access. Adds the component if it doesn't exist.
    template <class T>
    T& operator[](const T* ptr)
    {
      auto component = GetComponent<T>();
      if (component == nullptr)
      {
        component = &AddComponent<T>();
      }
      return *component;
    }

  private: // methods

    // Destroys all components and children.
    void Clear()
    {
      children.clear();
      components.clear();
      toDestroy.clear();
      destroyFlag = false;
      name.clear();
    }

    // Copies all child game objects from an array.
    void CopyChildren(const vector<unique_ptr<GameObject>>& objects)
    {
      for (auto& object : objects)
      {
        StoreChild(make_unique<GameObject>(*object));
      }
    }

    // Copies all components from an array.
    void CopyComponents(const vector<unique_ptr<IComponent>>& components)
    {
      for (auto& component : components)
      {
        StoreComponent(component->Clone());
      }
    }

    // Generates a new unique identifier for the game object.
    static uint64_t GenerateIdentifier()
    {
      static uint64_t id = 0;
      return id++;
    }

    // Adds a child object directly into the current list of objects.
    GameObject& StoreChild(unique_ptr<GameObject> object)
    {
      children.push_back(move(object));
      return *children.back();
    }

    // Adds a component directly into the current list of components.
    IComponent& StoreComponent(unique_ptr<IComponent> component)
    {
      components.push_back(move(component));
      return *components.back();
    }
  };
} // namespace lite