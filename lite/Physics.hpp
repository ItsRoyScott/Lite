#pragma once

#include "CollisionDetector.hpp"
#include "CollisionPrimitives.hpp"
#include "ContactResolver.hpp"
#include "D3DInclude.hpp"
#include "Essentials.hpp"
#include "PhysicsRigidBody.hpp"

//================================================================================================//
// This engine is an implementation of "Game Physics Engine Development" by Ian Millington using  //
//  the XNA Math library (now replaced by DirectXMath).                                           //
//                                                                                                //
// Amazon: http://www.amazon.com/Game-Physics-Engine-Development-Commercial-Grade/dp/0123819768   //
// Cyclone Physics: https://github.com/idmillington/cyclone-physics                               //
//                                                                                                //
// The amount of math going on in this engine can be overwhelming at first. I recommend getting   //
//  the book or downloading the Cyclone engine and taking it piecemeal.                           //
//================================================================================================//

namespace lite
{
  class Physics : public LightSingleton<Physics>
  {
  private: // data

    // Whether a gravity actor should be added to bodies by default.
    bool addDefaultGravity = true;

    // Array of rigid bodies.
    vector<shared_ptr<PhysicsRigidBody>> bodies;

    // Stores all contacts and basic properties for this frame.
    CollisionData collisionData;

    // Array of all collision primitives.
    vector<shared_ptr<CollisionPrimitive>> collisionPrimitives;

    // Actor attached to bodies by default.
    function<void(PhysicsRigidBody&, float)> defaultGravityActor;

    // Resolves collisions reported by the CollisionDetector.
    ContactResolver resolver;

  public: // data

    // Number of times to run the entire scene simulation.
    //  Running multiple times will prevent objects from flying
    //  through each other.
    size_t SimulationIterations = 5;

  public: // methods

    Physics(bool addGravity = true, float3 defaultGravityVector = { 0, -9.8f, 0 }) :
      addDefaultGravity(addGravity)
    {
      defaultGravityActor = [=](PhysicsRigidBody& p, float) { p.AddForce(defaultGravityVector); };
    }

    template <class T>
    shared_ptr<T> AddCollisionPrimitive()
    {
      shared_ptr<T> ptr = { Align<16>::New<T>(), Align<16>::Delete<T> };

      // Create the new primitive.
      collisionPrimitives.push_back(ptr);

      return move(ptr);
    }

    shared_ptr<PhysicsRigidBody> AddRigidBody()
    {
      // Create the new body.
      bodies.emplace_back(Align<16>::New<PhysicsRigidBody>(), Align<16>::Delete<PhysicsRigidBody>);
      shared_ptr<PhysicsRigidBody>& body = bodies.back();
      
      // Add default gravity actor if it was requested at startup.
      if (addDefaultGravity)
      {
        body->Actors.push_back(defaultGravityActor);
      }

      return body;
    }

    void Update(float dt)
    {
      // Divide the dt for multiple simulations.
      dt /= (float)SimulationIterations;

      // Perform the simulation as many times as requested. The more
      //  iterations the less likely objects will fly through each other.
      for (size_t i = 0U; i < SimulationIterations; ++i)
      {
        // Integrate all bodies.
        for (auto& body : bodies)
        {
          body->ApplyActors(dt);
          body->Integrate(dt);
        }

        // Generate contacts.
        size_t contacts = GenerateContacts();

        // Resolve contacts.
        ContactResolver resolver;
        resolver.PositionIterations = contacts * 2;
        resolver.VelocityIterations = contacts * 2;
        resolver.ResolveContacts(collisionData.Contacts, dt);
      }

      // Clear accumulators for all bodies.
      for (auto& body : bodies)
      {
        // Reset forces applied.
        body->ClearAccumulators();
      }
    }

  private: // methods

    size_t GenerateContacts()
    {
      // Initialize all primitives.
      for (auto& primitive : collisionPrimitives)
      {
        primitive->CalculateInternals();
      }

      // Set up the collision data.
      collisionData.Clear();
      collisionData.Friction = 0.8f;
      collisionData.Restitution = 0.2f;
      collisionData.Tolerance = 0.1f;

      size_t total = 0;

      // Collide all registered primitives.
      for (size_t j = 0; j < collisionPrimitives.size(); ++j)
      {
        for (size_t i = j + 1; i < collisionPrimitives.size(); ++i)
        {
          CollisionPrimitive& a = *collisionPrimitives[i];
          CollisionPrimitive& b = *collisionPrimitives[j];
          total += CollisionDetector::Instance().Collide(a, b, collisionData);
        }
      }

      return total;
    }
  };
} // namespace lite