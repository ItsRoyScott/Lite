#pragma once

#include "Component.hpp"
#include "GameObject.hpp"
#include "Physics.hpp"
#include "Transform.hpp"

namespace lite
{
  class RigidBody : public Component<RigidBody>
  {
  private: // data

    shared_ptr<PhysicsRigidBody> body;
    bool pushedInitialTransform = false;

  public: // data

    // Mass of the object in kilograms.
    float Mass() const { return body->Mass(); }
    void Mass(float m) { body->SetMass(m); }

  public: // methods

    RigidBody()
    {
      body = Physics::CurrentInstance()->AddRigidBody();
    }

    RigidBody(const RigidBody& b)
    {
      body = Physics::CurrentInstance()->AddRigidBody();

      Mass(b.Mass());
    }

    void AddForce(const float3& f) 
    { 
      body->AddForce(f); 
    }

    void AttachToPrimitive(CollisionPrimitive& primitive)
    {
      primitive.Body = body.get();
    }

  private: // methods

    void PullFromSystems() override
    {
      // Publish physics update to the Transform component.
      Transform& tfm = OwnerReference()[Transform_];
      tfm.LocalPosition = body->Position();
      tfm.LocalRotation = body->Orientation();
    }

    void PushToSystems() override
    {
      if (!pushedInitialTransform)
      {
        Transform& tfm = OwnerReference()[Transform_];
        body->Initialize(tfm.LocalPosition, tfm.LocalRotation);
        pushedInitialTransform = true;
      }
    }
  };

  reflect(RigidBody);
} // namespace lite