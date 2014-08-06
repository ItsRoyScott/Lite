#pragma once

#include "FieldInfo.hpp"
#include "MethodInfo.hpp"
#include "ReflectionPlugin.hpp"

namespace lite
{
  // Meta information regarding a type in C++.
  class TypeInfo
  {
  private: // data

    const type_info*    cppType = nullptr;
    function<Variant()> create;
    void              (*deleter)(void*) = nullptr;
    vector<FieldInfo>   fields;
    bool                isReference = false;
    vector<MethodInfo>  methods;
    string              name = "NotBoundToReflection";
    ostream&          (*print)(ostream&, const void*) = nullptr;
    istream&          (*read)(istream&, void*) = nullptr;
    size_t              size = 0;
    const TypeInfo*     valueType = nullptr;

    friend class Reflection;

  public: // properties

    // Array of this type's fields.
    const vector<FieldInfo>& Fields = fields;

    // Whether this type represents a reference to a value-type.
    const bool& IsReference = isReference;

    // Array of this type's methods.
    const vector<MethodInfo>& Methods = methods;

    // Name of the type.
    const string& Name = name;

    // Size of the type in bytes.
    const size_t& Size = size;

    // Returns the value-type if this type is a reference.
    const TypeInfo* const& ValueType = valueType;

  public: // const methods

    Variant Create() const
    {
      // Verify that the default constructor exists.
      if (Methods.empty() || Methods[0].Name != Name || Methods[0].ArgumentTypes.size() != 0)
      {
        Warn("Type " << Name << " does not have a valid default constructor");
        return {};
      }

      // Get the constructor as a std::function.
      auto ctor = Methods[0].AsFunction<Variant()>();
      if (!ctor)
      {
        Warn("Could not retrieve default constructor for " << Name);
        return {};
      }

      return ctor();
    }

    // Finds a field using a predicate function. The signature
    //  of the predicate is bool(const FieldInfo&).
    template <class Predicate>
    const FieldInfo* FindFieldBy(Predicate pred) const
    {
      for (const FieldInfo& field : fields)
      {
        if (pred(field)) return &field;
      }
      return nullptr;
    }

    // Finds a method using a predicate function. The signature
    //  of the predicate is bool(const MethodInfo&).
    template <class Predicate>
    const MethodInfo* FindMethodBy(Predicate pred) const
    {
      for (const MethodInfo& method : methods)
      {
        if (pred(method)) return &method;
      }
      return nullptr;
    }

    // Finds a field by name. (May return null)
    const FieldInfo* GetField(const string& name) const
    {
      return FindFieldBy([&](const FieldInfo& f) { return f.Name == name; });
    }

    // Finds a method by name. (May return null)
    const MethodInfo* GetMethod(const string& name) const
    {
      return FindMethodBy([&](const MethodInfo& m) { return m.Name == name; });
    }

  public: // non-const methods

    TypeInfo() = default;
    TypeInfo(const TypeInfo&) = delete;
    TypeInfo& operator=(const TypeInfo&) = delete;
    ~TypeInfo() = default;

    // Add base-case; does nothing.
    template <class T>
    void Add()
    {}

    // Adds a field given its name and pointer.
    template <class T, class ValueT, class... Args>
    void Add(string name, ValueT* ptr, Args&&... args)
    {
      GetTypeBuilder<T>().NewStaticField(name, ptr);
      fields.emplace_back(move(name), ptr);
      Add<T>(forward<Args>(args)...);
    }

    template <class T, class ValueT, class... Args>
    void Add(string name, ValueT T::*fieldPtr, Args&&... args)
    {
      GetTypeBuilder<T>().NewField(name, fieldPtr);
      fields.emplace_back(move(name), fieldPtr);
      Add<T>(forward<Args>(args)...);
    }

    // Adds a static constructor given its name and pointer.
    template <class T, class... FuncArgs, class... Args>
    void Add(Variant(*ptr)(FuncArgs...), Args&&... args)
    {
      GetTypeBuilder<T>().NewConstructor(Name, ptr);
      methods.emplace_back(Name, ptr);
      Add<T>(forward<Args>(args)...);
    }

    // Adds a static function given its name and pointer.
    template <class T, class RetT, class... FuncArgs, class... Args>
    void Add(string name, RetT(*ptr)(FuncArgs...), Args&&... args)
    {
      GetTypeBuilder<T>().NewStaticFunction(name, ptr);
      methods.emplace_back(move(name), ptr);
      Add<T>(forward<Args>(args)...);
    }

    // Adds a member function given its name and pointer.
    template <class T, class ClassT, class RetT, class... FuncArgs, class... Args>
    void Add(string name, RetT(ClassT::*ptr)(FuncArgs...), Args&&... args)
    {
      GetTypeBuilder<T>().NewMethod(name, ptr);
      methods.emplace_back(move(name), ptr);
      Add<T>(forward<Args>(args)...);
    }

    // Adds a const member function given its name and pointer.
    template <class T, class ClassT, class RetT, class... FuncArgs, class... Args>
    void Add(string name, RetT(ClassT::*ptr)(FuncArgs...) const, Args&&... args)
    {
      GetTypeBuilder<T>().NewMethod(name, ptr);
      methods.emplace_back(move(name), ptr);
      Add<T>(forward<Args>(args)...);
    }

    template <class T, class FieldT1, class T1, class FieldT2, class T2, class... Args>
    void Add(string name, FieldT1(T1::*getter)() const, void(T2::*setter)(FieldT2), Args&&... args)
    {
      // Notify the plugin of the new field.
      GetTypeBuilder<T>().NewProperty(name, getter, setter);

      // Create the field.
      fields.emplace_back(move(name), getter, setter);

      // Perfect-forward the rest of the arguments to Add.
      Add<T>(forward<Args>(args)...);
    }

    template <class T, class FieldT, class ClassT, class... Args>
    void Add(string name, FieldT(ClassT::*getter)(), struct ReadOnly_*, Args&&... args)
    {
      // Notify the plugin of the new field.
      GetTypeBuilder<T>().NewReadOnlyProperty(name, getter);
      
      // Create the field.
      fields.emplace_back(move(name), getter);
      
      // Perfect-forward the rest of the arguments to Add.
      Add<T>(forward<Args>(args)...);
    }

    template <class T, class FieldT, class ClassT, class... Args>
    void Add(string name, FieldT(ClassT::*getter)() const, struct ReadOnly_*, Args&&... args)
    {
      // Notify the plugin of the new field.
      GetTypeBuilder<T>().NewReadOnlyProperty(name, getter);

      // Create the field.
      fields.emplace_back(move(name), getter);

      // Perfect-forward the rest of the arguments to Add.
      Add<T>(forward<Args>(args)...);
    }

    template <class T, class FieldT1, class FieldT2, class... Args>
    void Add(string name, FieldT1(*getter)(), void(*setter)(FieldT2), Args&&... args)
    {
      // Notify the plugin of the new property.
      GetTypeBuilder<T>().NewStaticProperty(name, getter, setter);

      // Create the field.
      fields.emplace_back(move(name), getter, setter);

      // Perfect-forward the rest of the arguments to Add.
      Add<T>(forward<Args>(args)...);
    }

    template <class T, class FieldT1, class... Args>
    void Add(string name, FieldT1(*getter)(), struct ReadOnly_*, Args&&... args)
    {
      // Notify the plugin of the new property.
      GetTypeBuilder<T>().NewStaticReadOnlyProperty(name, getter);

      // Create the field.
      fields.emplace_back(move(name), getter);

      // Perfect-forward the rest of the arguments to Add.
      Add<T>(forward<Args>(args)...);
    }

    // Calls Add functions recursively to initialize the type.
    template <class T, class... Args>
    void Bind(Args&&... args)
    {
      cppType = &typeid(T);
      name = GetTypeName(typeid(T));
      print = Variant::GeneratePrintFunction<T>();
      read = Variant::GenerateReadFunction<T>();

      NotifyPluginOnBeginType<T>(name);

      Add<T>(forward<Args>(args)...);

      NotifyPluginOnEndType<T>(name);
    }

    // Initializes this type as a reference-type.
    template <class T>
    void InitializeReferenceType()
    {
      isReference = true;
      valueType = &TypeOf<T>();
    }

    // Formats the type info into an ostream.
    friend ostream& operator<<(ostream& os, const TypeInfo& ti)
    {
      os << ti.Name;
    }

  private: // methods

    static string GetTypeName(const type_info& typeInfo)
    {
      string name = typeInfo.name();

      // Take everything after the first namespace.
      auto find = name.find("::");
      if (find != string::npos)
      {
        name = name.substr(find + sizeof("::")-1);
      }

      return move(name);
    }

    // Notifies the plugin that we are starting to bind a class type.
    template <class T>
    typename enable_if<is_class<T>::value>::type
      NotifyPluginOnBeginType(const string& name)
    {
      GetTypeBuilder<T>().BeginClassType(name);
    }

    // Notifies the plugin that we are starting to bind a value type.
    template <class T>
    typename enable_if<!is_class<T>::value>::type
      NotifyPluginOnBeginType(const string& name)
    {
      GetTypeBuilder<T>().BeginValueType(name);
    }

    // Notifies the plugin that we are finishing a class type.
    template <class T>
    typename enable_if<is_class<T>::value>::type
      NotifyPluginOnEndType(const string& name)
    {
      GetTypeBuilder<T>().EndClassType(name);
    }

    // Notifies the plugin that we are finishing a value type.
    template <class T>
    typename enable_if<!is_class<T>::value>::type
      NotifyPluginOnEndType(const string& name)
    {
      GetTypeBuilder<T>().EndValueType(name);
    }
  };

  namespace detail
  {
    // Stores the static data for a type info.
    template <class T>
    TypeInfo& TypeOf()
    {
      static TypeInfo type;
      return type;
    }
  } // namespace detail

  // Returns a TypeInfo given its C++ type.
  template <class T>
  const TypeInfo& TypeOf()
  {
    return detail::TypeOf<typename decay<T>::type>();
  }
} // namespace lite