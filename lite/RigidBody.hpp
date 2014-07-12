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

    friend class SphereCollision;

    shared_ptr<PhysicsRigidBody> body;

  public: // data

    // Mass of the object in kilograms.
    float Mass() const { return body->Mass(); }
    void Mass(float m) { body->SetMass(m); }

  public: // methods

    RigidBody()
    {
      body = Physics::CurrentInstance()->AddRigidBody();
    }

    void Initialize() override
    {
      Transform& tfm = OwnerReference()[Transform_];
      body->Initialize(tfm.LocalPosition, tfm.LocalRotation);
    }

  private: // methods

    void PullFromSystems() override
    {
      // Publish physics update to the Transform component.
      Transform& tfm = OwnerReference()[Transform_];
      tfm.LocalPosition = body->Position();
      tfm.LocalRotation = body->Orientation();
    }
  };
} // namespace lite