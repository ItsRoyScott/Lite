#pragma once

#include "Contact.hpp"
#include "Essentials.hpp"
#include "float4x4.hpp"
#include "PhysicsRigidBody.hpp"
#include <unordered_map>

namespace lite
{
  class CollisionPrimitive
  {
  public: // types

    enum PrimitiveType
    {
      Plane,
      Sphere,
      Count
    };

  private: // data

    float4x4 transform;
    PrimitiveType type;

  public: // properties

    PhysicsRigidBody* Body = nullptr;
    float4x4 OffsetFromBody;

    const PrimitiveType& Type() const { return type; }

  public: // methods

    CollisionPrimitive(PrimitiveType type_) :
      type(type_)
    {}

    virtual ~CollisionPrimitive() {}

    void CalculateInternals()
    {
      transform = Body->Transform() * OffsetFromBody;
    }

    float4 GetAxis(size_t idx) const
    {
      return Matrix(transform).GetAxisVector(idx);
    }

    const float4x4& GetTransform() const
    {
      return transform;
    }
  };

  typedef CollisionPrimitive::PrimitiveType CollisionType;

  class CollisionPlane : public CollisionPrimitive
  {
  public: // data

    float3 Direction;
    float Offset;

  public: // methods

    CollisionPlane() :
      CollisionPrimitive(CollisionType::Plane)
    {}
  };

  class CollisionSphere : public CollisionPrimitive
  {
  public: // data

    float Radius = 1;

  public: // methods

    CollisionSphere() :
      CollisionPrimitive(CollisionType::Sphere)
    {}
  };

} // namespace lite