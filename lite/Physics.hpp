#pragma once

#include "CollisionDetector.hpp"
#include "CollisionPrimitives.hpp"
#include "Contact.hpp"
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
  class ContactResolver
  {
  private: // data

    size_t positionIterationsUsed = 0;

    size_t velocityIterationsUsed = 0;

  public: // data

    float PositionEpsilon = 0.01f;

    size_t PositionIterations = 0;

    float VelocityEpsilon = 0.01f;

    size_t VelocityIterations = 0;

  public: // methods

    void ResolveContacts(aligned_vector<Contact>& contacts, float dt)
    {
      // Make sure we have something to do.
      if (contacts.size() == 0) return;

      // Prepare the contacts for processing.
      for (auto& contact : contacts)
      {
        contact.CalculateInternals(dt);
      }

      // Resolve the interpenetration problems with the contacts.
      AdjustPositions(contacts.data(), contacts.size(), dt);

      // Resolve the velocity problems with the contacts.
      AdjustVelocities(contacts.data(), contacts.size(), dt);
    }

  private: // methods

    void AdjustPositions(Contact* contacts, size_t numContacts, float dt)
    {
      unsigned i, index;
      float3 linearChange[2], angularChange[2];
      float max;
      Vector deltaPosition;

      // iteratively resolve interpenetrations in order of severity.
      for (positionIterationsUsed = 0; positionIterationsUsed < PositionIterations; ++positionIterationsUsed)
      {
        // Find biggest penetration
        max = PositionEpsilon;
        index = numContacts;
        for (i = 0; i < numContacts; i++)
        {
          if (contacts[i].Penetration > max)
          {
            max = contacts[i].Penetration;
            index = i;
          }
        }
        if (index == numContacts) break;

        // Match the awake state at the contact.
        contacts[index].MatchAwakeState();

        // Resolve the penetration.
        contacts[index].ApplyPositionChange(linearChange, angularChange, max);

        // Again this action may have changed the penetration of other
        // bodies, so we update contacts.
        for (i = 0; i < numContacts; i++)
        {
          // Check each body in the contact.
          for (unsigned b = 0; b < 2; b++)
          {
            if (contacts[i].Body[b])
            {
              // Check for a match with each body in the newly resolved contact.
              for (unsigned d = 0; d < 2; d++)
              {
                if (contacts[i].Body[b] == contacts[index].Body[d])
                {
                  deltaPosition = Vector(linearChange[d]) +
                    Vector(angularChange[d]).Cross(contacts[i].RelativeContactPosition[b]);

                  // The sign of the change is positive if we're dealing with 
                  //   the second body in a contact and negative otherwise.
                  //   (because we're subtracting the resolution)
                  contacts[i].Penetration += 
                    deltaPosition.Dot(contacts[i].ContactNormal) * (b ? 1 : -1);
                }
              }
            }
          }
        }
      }
    }

    void AdjustVelocities(Contact* contacts, size_t numContacts, float dt)
    {
      Vector velocityChange[2], rotationChange[2];
      Vector deltaVel;

      // iteratively handle impacts in order of severity.
      for (velocityIterationsUsed = 0; velocityIterationsUsed < VelocityIterations; ++velocityIterationsUsed)
      {
        // Find contact with maximum magnitude of probable velocity change.
        float max = VelocityEpsilon;
        unsigned index = numContacts;
        for (unsigned i = 0; i < numContacts; i++)
        {
          if (contacts[i].DesiredDeltaVelocity > max)
          {
            max = contacts[i].DesiredDeltaVelocity;
            index = i;
          }
        }
        if (index == numContacts) break;

        // Match the awake state at the contact
        contacts[index].MatchAwakeState();

        // Do the resolution on the contact that came out top.
        contacts[index].ApplyVelocityChange(velocityChange, rotationChange);

        // With the change in velocity of the two bodies, the update of
        // contact velocities means that some of the relative closing
        // velocities need recomputing.
        for (unsigned i = 0; i < numContacts; i++)
        {
          // Check each body in the contact:
          for (unsigned b = 0; b < 2; b++)
          {
            if (contacts[i].Body[b])
            {
              // Check for a match with each body in the newly resolved contact:
              for (unsigned d = 0; d < 2; d++)
              {
                if (contacts[i].Body[b] == contacts[index].Body[d])
                {
                  deltaVel = velocityChange[d] + 
                    rotationChange[d].Cross(contacts[i].RelativeContactPosition[b]);

                  // The sign of the change is negative if we're dealing
                  // with the second body in a contact.
                  contacts[i].ContactVelocity = contacts[i].ContactVelocity +
                    contacts[i].ContactToWorld.TransformTranspose(deltaVel)
                    * (b ? -1.0f : 1.0f);
                  contacts[i].CalculateDesiredDeltaVelocity(dt);
                }
              }
            }
          }
        }
      }
    }
  };

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
      // Divide the dt in half for two simulations.
      dt /= 2;

      // Perform the simulation twice to avoid objects falling through
      //  other objects. Another solution would be to implement
      //  "continuous" physics.
      for (int i = 0; i < 2; ++i)
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