#pragma once

#include "Component.hpp"
#include "DebugDrawer.hpp"
#include "GameObject.hpp"
#include "Physics.hpp"
#include "RigidBody.hpp"
#include "Transform.hpp"

namespace lite
{
  template <class T, class Primitive>
  class CollisionComponent : public Component<T>
  {
  protected: // data

    GOId objectWithRigidBody;
    shared_ptr<Primitive> primitive;

  protected: // methods

    CollisionComponent()
    {
      primitive = Physics::CurrentInstance()->AddCollisionPrimitive<Primitive>();
    }

    CollisionComponent(const CollisionComponent& b)
    {
      primitive = Physics::CurrentInstance()->AddCollisionPrimitive<Primitive>();
    }

    void Initialize() override
    {
      UpdateOwningRigidBody();
    }

    void PushToSystems() override
    {
      RigidBody* rigidBody = UpdateOwningRigidBody();
      if (rigidBody)
      {
        // Update the primitive's offset transform from the rigid body.
        Transform& thisTfm = OwnerReference()[Transform_];
        Transform& bodyTfm = rigidBody->OwnerReference()[Transform_];
        primitive->OffsetFromBody = thisTfm.GetOffsetFromParent(bodyTfm);
      }
    }

    RigidBody* UpdateOwningRigidBody()
    {
      // Search upwards for the owning rigid body.
      RigidBody* rigidBody = OwnerReference().GetComponentUpwards<RigidBody>();
      if (!rigidBody)
      {
        primitive->Body = nullptr;
        return nullptr;
      }

      // Check for first initialization.
      if (!objectWithRigidBody)
      {
        objectWithRigidBody = rigidBody->OwnerReference().Identifier();
        rigidBody->AttachToPrimitive(*primitive);
        return rigidBody;
      }

      // Check if the owning rigid body has changed.
      if (objectWithRigidBody != rigidBody->OwnerReference().Identifier())
      {
        // Re-assign the body to the collision primitive.
        rigidBody->AttachToPrimitive(*primitive);
        return rigidBody;
      }

      return rigidBody;
    }
  };

  class PlaneCollision : public CollisionComponent<PlaneCollision, CollisionPlane>
  {
  public: // properties

    const float3& Direction() const { return primitive->Direction; }
    void Direction(const float3& d) { primitive->Direction = d; }

    const float& Offset() const { return primitive->Offset; }
    void Offset(float f) { primitive->Offset = f; }
  };

  reflect(PlaneCollision,
    "Direction", Getter(&T::Direction), Setter(&T::Direction),
    "Offset", Getter(&T::Offset), Setter(&T::Offset));

  class SphereCollision : public CollisionComponent < SphereCollision, CollisionSphere >
  {
  private: // data

    float radius = 1;

  public: // properties

    const float& Radius() const { return radius; }
    void Radius(float f) { radius = f; }

    void PushToSystems() override
    {
      CollisionComponent::PushToSystems();

      // Update the primitive's radius using the maximum of transform's scale.
      Transform& tfm = OwnerReference()[Transform_];
      primitive->Radius = max(tfm.LocalScale.x, max(tfm.LocalScale.y, tfm.LocalScale.z)) * radius;
    }
  };

  reflect(SphereCollision,
    "Radius", Getter(&T::Radius), Setter(&T::Radius));
} // namespace lite