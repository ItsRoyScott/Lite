#pragma once

#include "Component.hpp"
#include "GameObject.hpp"
#include "Physics.hpp"
#include "RigidBody.hpp"
#include "Transform.hpp"

namespace lite
{
  class SphereCollision : public Component < SphereCollision >
  {
  private: // data

    GOId objectWithRigidBody;
    shared_ptr<CollisionSphere> sphere;

  public: // properties

    float Radius() const { return sphere->Radius; }
    void Radius(float f) { sphere->Radius = f; }

  public: // methods

    SphereCollision()
    {
      sphere = Physics::CurrentInstance()->AddCollisionPrimitive<SphereCollision>();
    }

  private: // methods

    void Initialize() override
    {
      UpdateOwningRigidBody();
    }

    void PushToSystems() override
    {
      RigidBody* rigidBody = UpdateOwningRigidBody();
      if (rigidBody)
      {
        Transform& thisTfm = OwnerReference()[Transform_];
        Transform& bodyTfm = rigidBody->OwnerReference()[Transform_];
        sphere->Offset = thisTfm.GetOffsetFromParent(bodyTfm);
      }
    }

    RigidBody* UpdateOwningRigidBody()
    {
      // Search upwards for the owning rigid body.
      RigidBody* rigidBody = OwnerReference().GetComponentUpwards<RigidBody>();
      if (!rigidBody)
      {
        sphere->Body = nullptr;
        return nullptr;
      }

      // Check for first initialization.
      if (!objectWithRigidBody)
      {
        objectWithRigidBody = rigidBody->OwnerReference().Identifier();
        sphere->Body = rigidBody->body.get();
        return rigidBody;
      }

      // Check if the owning rigid body has changed.
      if (objectWithRigidBody != rigidBody->OwnerReference().Identifier())
      {
        // Re-assign the body to the collision primitive.
        sphere->Body = rigidBody->body.get();
        return rigidBody;
      }
    }
  };
} // namespace lite