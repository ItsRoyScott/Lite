#pragma once

#include "Contact.hpp"
#include "Essentials.hpp"
#include "float4x4.hpp"
#include "PhysicsRigidBody.hpp"
#include <unordered_map>

namespace lite
{
  // Base class for all collision primitives.
  class CollisionPrimitive
  {
  public: // types

    // Enumeration of all possible primitive types.
    enum PrimitiveType
    {
      Plane,
      Sphere,
      Count
    };

  private: // data
    
    Matrix        transform;
    PrimitiveType type;

  public: // data

    // Pointer to the associated rigid body.
    PhysicsRigidBody* Body = nullptr;

    // Transformational offset from the rigid body.
    float4x4 OffsetFromBody;

  public: // properties

    // Returns the type of the primitive.
    const PrimitiveType& Type() const { return type; }

  public: // methods

    // Assigns the type of the primitive.
    CollisionPrimitive(PrimitiveType type_) :
      type(type_)
    {}

    virtual ~CollisionPrimitive() {}

    // Calculates the true transform of this primitive.
    void CalculateInternals()
    {
      transform = Body->Transform() * OffsetFromBody;
    }

    // Returns an axis of the transform matrix. 
    //  e.g. GetAxis(3) returns the position.
    float4 GetAxis(size_t idx) const
    {
      return transform.GetAxisVector(idx);
    }

    // Returns the final transform of the primitive.
    const Matrix& GetTransform() const
    {
      return transform;
    }
  };

  // Shorthand for the type of a collision primitive.
  typedef CollisionPrimitive::PrimitiveType CollisionType;

  // Represents a plane that objects can collide against.
  class CollisionPlane : public CollisionPrimitive
  {
  public: // data

    // Direction of the plane.
    float3 Direction;

    // Offset in world space using the direction vector.
    float Offset;

  public: // methods

    CollisionPlane() :
      CollisionPrimitive(CollisionType::Plane)
    {}
  };

  // Represents a sphere that objects can collide against.
  class CollisionSphere : public CollisionPrimitive
  {
  public: // data

    // Half-width of the sphere.
    float Radius = 1;

  public: // methods

    CollisionSphere() :
      CollisionPrimitive(CollisionType::Sphere)
    {}
  };

} // namespace lite