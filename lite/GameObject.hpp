#pragma once

#include "Component.hpp"
#include "ComponentManager.hpp"
#include "Essentials.hpp"
#include "EventHandler.hpp"

namespace lite
{
  class GameObject
  {
  private: // data

    vector<unique_ptr<GameObject>> children;
    vector<unique_ptr<IComponent>> components;
    bool  destroyFlag = false;
    uint32_t identifier = GenerateIdentifier();
    bool isActive = true;
    string name;
    GameObject* parent = nullptr;
    vector<GameObject*> toDestroy;

  public: // properties

    // Whether the game object and its components are updating.
    const bool& Active() const { return isActive; }
    // Set whether the game object and its components should update.
    void Active(bool isActive_) 
    { 
      isActive = isActive_;
      for (auto& component : components)
      {
        component->SetActive(isActive);
      }
    }

    // Children game objects attached to this game object.
    const vector<unique_ptr<GameObject>>& Children() const { return children; }

    // Whether this object will be destroyed at the end of the frame.
    const bool& DestroyFlag() const { return destroyFlag; }

    // Unique identifer of this game object.
    const uint32_t& Identifier() const { return identifier; }

    // Name of this game object. (May be empty)
    const string& Name() const { return name; }
    // Changes the name of the game object.
    void Name(string name_) { name = move(name_); }

    // Pointer to the parent game object. (May be null)
    GameObject* const& Parent() const { return parent; }

    // Recommended way to access parent, because 
    //  an assertion will check for null dereference.
    GameObject& ParentReference() const
    {
      FatalIf(!parent, "Dereferencing null parent GameObject");
      return *parent;
    }

  public: // methods

    explicit GameObject(bool active = true) :
      isActive(active),
      name("GO" + to_string(identifier))
    {
      Instances()[identifier] = this;
    }

    GameObject(GameObject&& b) :
      children(move(b.children)),
      components(move(b.components)),
      destroyFlag(b.destroyFlag),
      identifier(b.identifier),
      isActive(b.isActive),
      name(move(b.name)),
      parent(b.parent),
      toDestroy(move(b.toDestroy))
    {
      Instances()[identifier] = this;
    }

    GameObject(const GameObject& b) :
      name(b.name)
    {
      Instances()[identifier] = this;
      CopyChildren(b.children);
      CopyComponents(b.components);
    }

    virtual ~GameObject() 
    {
      Clear();
      Instances().erase(identifier);
    }

    GameObject& operator=(GameObject&& b)
    {
      children = move(b.children);
      components = move(b.components);
      destroyFlag = b.destroyFlag;
      isActive = b.isActive;
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
    GameObject& AddChild(const GameObject& prefab = GameObject(), bool initialize = true)
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
      return static_cast<T&>(AddComponent(TypeOf<T>().Name, initialize));
    }

    // Adds a new component by name.
    IComponent& AddComponent(const string& name)
    {
      return AddComponent(name, true);
    }

    // Adds a new component by name.
    IComponent& AddComponent(const string& name, bool initialize)
    {
      // Call on the component manager to create the component.
      IComponent& component = StoreComponent(ComponentManager::Instance().Create(name));

      // Initialize: 'initialize' will be false if we are loading a level.
      if (initialize)
      {
        component.Initialize();

        // Propagate the active flag to the new component.
        component.SetActive(isActive);
      }

      return component;
    }

    // Deserializes formatted object data using the given input stream.
    //  The 'level' parameter can be ignored: it is used internally
    //  for keeping track of the current recursion level.
    istream& Deserialize(istream& is, size_t level = 0)
    {
      string s1, s2, s3;

      if (level == 0)
      {
        // Read the object group opening.
        is >> s1;
        if (s1 != "[") return is;

        // Read 'type = GameObject'.
        is >> s1 >> s2 >> s3;
        if (s1 != "type" || s2 != "=" || s3 != TypeOf<GameObject>().Name) return is;
      }

      // Read 'name = GOXXX'.
      is >> s1 >> s2 >> s3;
      if (s1 != "name" || s2 != "=") return is;
      name = s3;

      // For each child object or component.
      for (is >> s1; s1 == "["; is >> s1)
      {
        // Read the type.
        is >> s1 >> s2 >> s3;
        if (s1 != "type" || s2 != "=") continue;
        
        // If the group represents a child object:
        if (s3 == TypeOf<GameObject>().Name) 
        {
          // Add the object and deserialize it.
          GameObject& object = AddChild();
          object.Deserialize(is, level + 1);
          continue;
        }

        // Add the component using its type name.
        IComponent& component = AddComponent(s3);
        const TypeInfo& componentType = component.GetType();

        // Assume we just finished reading the component's fields.
        //  (This is checked after the while loop below.)
        s1 = "]";

        // For each of the component's fields:
        while (true)
        {
          // Read the name of the field.
          is >> s1;

          // Look up the field by name.
          const FieldInfo* componentField = componentType.GetField(s1);
          if (!componentField)
          {
            if (s1 == "]") break;
            else continue;
          }

          // Read the "=".
          is >> s1;
          if (s1 != "=")
          {
            if (s1 == "]") break;
            else continue;
          }

          // Create the field value as a variant. Use the variant's "read"
          //  capability to read from the istream.
          Variant fieldValue = componentField->Type->Create();
          is >> fieldValue;
          componentField->Set(fieldValue, &component);
        }

        if (s1 != "]")
        {
          // Read the component group closing bracket.
          is >> s1;
          if (s1 != "]") break;
        }
      }
      
      // Read the object group closing bracket.
      is >> s1;

      return is;
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

    // Finds a game object by its identifier. (May return null)
    static GameObject* FindByIdentifier(uint32_t id)
    {
      auto it = Instances().find(id);
      if (it == Instances().end()) return nullptr;
      return it->second;
    }

    // Finds a game object given a predicate condition. (May return null)
    //  Signature of the predicate is bool(GameObject&).
    template <class Predicate>
    GameObject* FindChildBy(Predicate pred)
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
    IComponent* FindComponentBy(Predicate pred)
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

    // Finds a component by searching recursively through owner objects.
    //  Signature of the predicate is bool(IComponent&).
    template <class Predicate>
    IComponent* FindComponentUpwardsBy(Predicate pred)
    {
      for (GameObject* owner = this; owner != nullptr; owner = owner->Parent())
      {
        IComponent* component = owner->FindComponentBy(pred);
        if (component)
        {
          return component;
        }
      }
      return nullptr;
    }

    // Finds a child by its name. (May return null)
    GameObject* GetChild(const string& name)
    {
      return FindChildBy([&](GameObject& object) { return object.Name() == name; });
    }

    // Finds a child by identifier. (May return null)
    GameObject* GetChild(uint64_t id)
    {
      return FindChildBy([&](GameObject& object) { return object.Identifier() == id; });
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
      IComponent* component =  FindComponentBy([](IComponent& component) 
      { 
        return &component.GetType() == &TypeOf<T>(); 
      });
      return static_cast<T*>(component);
    }

    // Returns a component by its type name. (May return null)
    IComponent* GetComponent(const string& typeName)
    {
      return FindComponentBy([&](IComponent& component) 
      { 
        return component.GetType().Name == typeName; 
      });
    }

    // Returns a component from type by searching recursively
    //  through owner objects. (May return null)
    template <class T>
    T* GetComponentUpwards()
    {
      IComponent* component = FindComponentUpwardsBy([](IComponent& component) 
      { 
        return &component.GetType() == &TypeOf<T>(); 
      });
      return static_cast<T*>(component);
    }

    // Returns a component from name by searching recursively
    //  through owner objects. (May return null)
    IComponent* GetComponentUpwards(const string& typeName)
    {
      return FindComponentUpwardsBy([&](IComponent& component)
      {
        return component.GetType().Name == typeName;
      });
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

    // Loads the game object from a file.
    bool LoadFromFile(const string& filename)
    {
      // Open the file.
      auto file = ifstream(config::Objects + filename);
      if (!file.is_open()) return false;

      // Deserialize using the ifstream as the input stream.
      Deserialize(file);

      return true;
    }

    // Calls PullFromSystems function on all components recursively.
    void PullFromSystems()
    {
      if (!isActive)
      {
        // Ensure components are notified that this object is inactive.
        Active(false);
      }

      // Call on all components.
      for (auto& component : components)
      {
        component->PullFromSystems();
      }

      // Call on all child objects.
      for (auto& child : children)
      {
        child->PullFromSystems();
      }
    }

    // Calls PullFromSystems function on all components recursively.
    void PushToSystems()
    {
      if (!isActive)
      {
        // Ensure components are notified that this object is inactive.
        Active(false);
      }

      // Call on all components.
      for (auto& component : components)
      {
        component->PushToSystems();
      }

      // Call on all child objects.
      for (auto& child : children)
      {
        child->PushToSystems();
      }
    }

    // Serializes this object, all components, and child objects to file.
    bool SaveToFile(const string& filename)
    {
      // Open the file.
      auto file = ofstream(config::Objects + filename);
      if (!file.is_open()) return false;

      // Serialize using the ofstream as an output stream.
      Serialize(file);

      return true;
    }

    // Serializes the object data to the given output stream.
    //  The 'level' parameter can be ignored: it is used internally
    //  for keeping track of tabs to make the output pretty.
    ostream& Serialize(ostream& os, size_t level = 0)
    {
      // Open the object group.
      os << Tabs(level++) << "[\n";

      // Serialize the type and name of the object.
      os << Tabs(level) << "type = " << TypeOf<GameObject>().Name << "\n";
      os << Tabs(level) << "name = " << Name() << "\n\n";

      // For each component:
      for (auto& component : components)
      {
        // Get the reflected type. (see Reflection)
        const TypeInfo& componentType = component->GetType();

        // Open the component group.
        os << Tabs(level++) << "[\n";
        // Write the component type name.
        os << Tabs(level) << "type = " << componentType.Name << "\n";

        // For each field:
        for (auto& componentField : componentType.Fields)
        {
          // Write the field name followed by its value(s).
          os << Tabs(level) << componentField.Name << " = " << componentField.Get(component.get()) << "\n";
        }

        // End the component group.
        os << Tabs(--level) << "]\n";
      }

      // For each child object:
      for (auto& child : children)
      {
        // Serialize the child.
        child->Serialize(os, level);
        os << "\n";
      }

      // Close the object group.
      return os << Tabs(--level) << "]";
    }

    // Updates child objects, then the components of this object.
    void Update()
    {
      if (!isActive)
      {
        // Ensure components are notified that this object is inactive.
        Active(false);
      }

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

    friend ostream& operator<<(ostream& os, const GameObject&)
    {
      return os;
    }

    friend istream& operator>>(istream& is, GameObject&)
    {
      return is;
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
    static uint32_t GenerateIdentifier()
    {
      static uint32_t id = 0;
      return id++;
    }

    // Stores a map of all GameObject instances by id.
    static unordered_map<uint32_t, GameObject*>& Instances()
    {
      static unordered_map<uint32_t, GameObject*> instances;
      return instances;
    }

    // Adds a child object directly into the current list of objects.
    GameObject& StoreChild(unique_ptr<GameObject> object)
    {
      children.push_back(move(object));
      children.back()->parent = this;
      return *children.back();
    }

    // Adds a component directly into the current list of components.
    IComponent& StoreComponent(unique_ptr<IComponent> component)
    {
      FatalIf(!component, "Null component being stored in GameObject");
      components.push_back(move(component));
      components.back()->SetOwner(*this);
      return *components.back();
    }
  };

  reflect(GameObject);

  class GOId
  {
  private: // data

    uint32_t id = (uint32_t)-1;

  public: // methods

    GOId() = default;

    GOId(uint32_t id_) : 
      id(id_)
    {}

    explicit operator bool() const
    {
      return GameObject::FindByIdentifier(id) != nullptr;
    }

    bool operator!() const
    {
      return !bool(*this);
    }

    operator GameObject*() const
    {
      return GameObject::FindByIdentifier(id);
    }

    bool operator==(const GOId& b) const
    {
      return id == b.id;
    }

    bool operator==(uint32_t id) const
    {
      return this->id == id;
    }

    bool operator!=(const GOId& b) const
    {
      return !(*this == b);
    }

    bool operator!=(uint32_t id) const
    {
      return !(*this == id);
    }
  };

} // namespace lite

namespace lite
{
  // A prefab is simply a deactivated GameObject.
  inline GameObject MakePrefab()
  {
    return GameObject(false);
  }
} // namespace lite