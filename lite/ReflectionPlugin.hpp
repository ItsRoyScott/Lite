#pragma once

#include "LuaCppInterfaceInclude.hpp"
#include "Scripting.hpp"
#include "Variant.hpp"

namespace lite
{
  // Hooks into the Reflection's binding process to automatically bind
  //  functions to Lua (using LuaBridge, for now).
  class ReflectionPlugin : public LightSingleton<ReflectionPlugin>
  {
  public: // types

    template <class T>
    class TypeBuilder : public LightSingleton<TypeBuilder<T>>
    {
    private: // data

      typedef luabridge::Namespace::Class<T> Class;
      typedef luabridge::Namespace Namespace;

      unique_ptr<Class> class_;
      unique_ptr<Namespace> namespace_;

    public: // methods

      TypeBuilder(ReflectionPlugin&)
      {}

      // Called when an object type bind is starting:
      //  Used to begin the class binding process through LuaBridge.
      void BeginClassType(const string& className)
      {
        lua_State* L = Scripting::Instance().L;

        namespace_.reset( new Namespace(luabridge::getGlobalNamespace(L).beginNamespace("lite")) );
        class_.reset( new Class(namespace_->beginClass<T>(className.c_str())) );
      }

      // Called when a non-object type (e.g. int, void*) bind is starting:
      //  Does nothing for now.
      void BeginValueType(const string&)
      {
      }

      // Called when an object type bind is ending:
      //  Used to end the class binding process through LuaBridge.
      void EndClassType(const string&)
      {
        class_->endClass();
        class_ = nullptr;
        namespace_ = nullptr;
      }

      // Called when a non-object type (e.g. int, void*) bind is ending:
      //  Does nothing for now.
      void EndValueType(const string&)
      {}

      // Called when a default constructor is added.
      void NewConstructor(const string&, Variant(*)())
      {
        class_->addConstructor<void(*)()>();
      }

      // Called when a constructor is added (non-variant type).
      template <class RetT, class... Args>
      void NewConstructor(const string&, RetT(*)(Args...))
      {}

      // Called when a member field is added.
      template <class Class, class Value>
      void NewField(const string& name, Value Class::*field)
      {
        Warn(name << " will not be bound to Lua. Use a custom property instead." <<
          "Fields do not work properly in LuaBridge. Instead of generating the getters and " <<
          "setters using references, they copy by-value. Therefore the field data will get " <<
          "a copy and the scripter will not be referring to the actual field's data as part " <<
          "of the class.");
      }

      // Called when a new member function is bound to the type:
      //  This adds the function to the LuaBridge Class object.
      template <class FuncPtr>
      void NewMethod(const string& name, FuncPtr funcPtr)
      {
        class_->addFunction(name.c_str(), funcPtr);
      }

      // Called when a new get-set pair is bound for a property.
      template <class GetterPtr, class SetterPtr>
      void NewProperty(const string& name, GetterPtr getter, SetterPtr setter)
      {
        // Add the getter/setter pair as functions because LuaBridge
        //  treats properties as values that can be copied, even when
        //  they use references.
        class_->addFunction(("Get" + name).c_str(), getter);
        class_->addFunction(("Set" + name).c_str(), setter);
      }

      // Called when a new non-const getter is bound for a read-only property.
      template <class Func>
      void NewReadOnlyProperty(const string& name, Func fn)
      {
        // Add the getter as a function because LuaBridge treats
        //  properties as values that can be copied, even when they
        //  use references.
        class_->addFunction(("Get" + name).c_str(), fn);
      }

      // Called when a new static field is bound to the type.
      template <class ValueT>
      void NewStaticField(const string& name, ValueT* ptr)
      {
        class_->addStaticData(name.c_str(), ptr);
      }

      // Called when a new static function is bound to the type.
      template <class RetT, class... Args>
      void NewStaticFunction(const string& name, RetT(*fn)(Args...))
      {
        class_->addStaticFunction(name.c_str(), fn);
      }

      // Called when a new static read-only property is bound to the type.
      template <class RetT>
      void NewStaticReadOnlyProperty(const string& name, RetT(*getter)())
      {
        class_->addStaticProperty(name.c_str(), getter);
      }
    };
  };

} // namespace lite