#pragma once

#include "Component.hpp"
#include "DebugDrawer.hpp"
#include "GameObject.hpp"
#include "Physics.hpp"
#include "RigidBody.hpp"
#include "Transform.hpp"

namespace lite
{
  static bool& DebugDrawCollisions()
  {
    static bool enabled = false;
    return enabled;
  }

  // Base class for all components which provide a collision primitive to the object.
  template <class T, class Primitive>
  class CollisionComponent : public Component<T>
  {
  protected: // data

    // The identifier of the object that holds the 
    //  RigidBody associated with this collider.
    GOId objectWithRigidBody;

    // Pointer to the collision primitive.
    shared_ptr<Primitive> primitive;

  protected: // methods

    // Calls on Physics to create the collision primitive.
    CollisionComponent()
    {
      primitive = Physics::CurrentInstance()->AddCollisionPrimitive<Primitive>();
    }

    // Calls on Physics to create the collision primitive.
    //  Note that 'objectWithRigidBody' is left to default-initialize.
    //  This forces the object to find its rigid body in Initialize.
    CollisionComponent(const CollisionComponent& b)
    {
      primitive = Physics::CurrentInstance()->AddCollisionPrimitive<Primitive>();
    }

    // Searches the object hierarchy upwards for the closest RigidBody.
    void Initialize() override
    {
      UpdateOwningRigidBody();
    }

    // Pushes the offset transform from the rigid body 
    //  to the underlying collision primitive.
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

    // Searches upwards for the closest associated rigid body and attaches
    //  this collision component to it.
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

  // Supports collisions with an infinite plane. Transform data is ignored
  //  for this type of collision.
  class PlaneCollision : public CollisionComponent<PlaneCollision, CollisionPlane>
  {
  public: // properties

    // Direction vector of the plane. Note that Transform orientation is ignored.
    const float3& Direction() const { return primitive->Direction; }
    void Direction(const float3& d) { primitive->Direction = d; }

    // Offset multiplied with the direction vector to indicate the position of the
    //  plane. Note the Transform position is ignored.
    const float& Offset() const { return primitive->Offset; }
    void Offset(float f) { primitive->Offset = f; }
  };

  // Bind PlaneCollision to reflection.
  template<>
  struct Binding<PlaneCollision> : BindingBase<PlaneCollision>
  {
    Binding()
    {
      Bind(
        "Direction", Const(&T::Direction), NonConst(&T::Direction),
        "Offset", Const(&T::Offset), NonConst(&T::Offset));
    }
  };

  // Supports sphere collisions.
  class SphereCollision : public CollisionComponent < SphereCollision, CollisionSphere >
  {
  private: // data

    float radius = 1;

  public: // properties

    // Determines the size of the sphere. This is multiplied
    //  with the max component of the Transform component's scale.
    const float& Radius() const { return radius; }
    void Radius(float f) { radius = f; }

  private: // methods

    // Sends the true radius of the sphere to physics.
    void PushToSystems() override
    {
      CollisionComponent::PushToSystems();

      // Update the primitive's radius using the maximum of transform's scale.
      Transform& tfm = OwnerReference()[Transform_];
      primitive->Radius = max(tfm.LocalScale.x, max(tfm.LocalScale.y, tfm.LocalScale.z)) * radius;

      if (DebugDrawCollisions())
      {
        auto pos = tfm.GetWorldMatrix().r[3];
        float3 posf;
        XMStoreFloat3(&posf, pos);
        DrawSphere(posf, { primitive->Radius*2, primitive->Radius*2, primitive->Radius*2 });
      }
    }
  };

  // Bind SphereCollision to reflection.
  template<>
  struct Binding<SphereCollision> : BindingBase<SphereCollision>
  {
    Binding()
    {
      Bind(
        "Radius", Const(&T::Radius), NonConst(&T::Radius));
    }
  };
} // namespace lite