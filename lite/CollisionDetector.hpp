#pragma once

#include "CollisionPrimitives.hpp"
#include "Essentials.hpp"
#include <unordered_map>

namespace lite
{
  class CollisionDetector : public Singleton < CollisionDetector >
  {
  public: // types

    typedef function<size_t(const CollisionPrimitive&, const CollisionPrimitive&, CollisionData&)> ContactGenerator;

  private: // data

    //unordered_map<type_index, unordered_map<type_index, ContactGenerator>> generatorMap;
    ContactGenerator generatorMap[CollisionType::Count][CollisionType::Count];

  public: // methods

    CollisionDetector()
    {
      // Add default generators.
      AddGenerator<CollisionSphere, CollisionPlane>(&SphereAndPlane);
      AddGenerator<CollisionSphere>(&SphereAndSphere);
    }

    // Adds a contact generator for colliding an object with itself.
    template <class AB>
    void AddGenerator(function<size_t(const AB&, const AB&, CollisionData&)> fn)
    {
      CollisionType type = AB().Type();

      ContactGenerator generator = reinterpret_cast<ContactGenerator&&>(fn);
      generatorMap[type][type] = move(generator);
    }

    // Adds a contact generator for colliding an object with another.
    template <class A, class B>
    void AddGenerator(function<size_t(const A&, const B&, CollisionData&)> fn)
    {
      CollisionType aType = A().Type();
      CollisionType bType = B().Type();

      ContactGenerator generator = reinterpret_cast<ContactGenerator&&>(fn);
      generatorMap[aType][bType] = generator;
      generatorMap[bType][aType] = generator;
    }

    size_t Collide(const CollisionPrimitive& a, const CollisionPrimitive& b, CollisionData& data)
    {
      const ContactGenerator& generator = generatorMap[a.Type()][b.Type()];
      if (generator)
      {
        return generator(a, b, data);
      }
      return 0;
    }

    static size_t SphereAndPlane(
      const CollisionSphere& sphere,
      const CollisionPlane& plane,
      CollisionData& data)
    {
      // Cache the sphere position.
      Vector position = sphere.GetAxis(3);

      // Find the distance from the plane
      float centerDistance = Vector(plane.Direction).Dot(position) - plane.Offset;

      // Check if we're within radius
      if (centerDistance*centerDistance > sphere.Radius * sphere.Radius) return 0;

      // Check which side of the plane we're on
      Vector normal = plane.Direction;
      float penetration = -centerDistance;
      if (centerDistance < 0)
      {
        normal *= -1;
        penetration = -penetration;
      }
      penetration += sphere.Radius;

      // Create the contact - it has a normal in the plane direction.
      Contact& contact = data.AddContact();
      contact.ContactNormal = normal;
      contact.Penetration = penetration;
      contact.ContactPoint = position - Vector(plane.Direction) * centerDistance;
      contact.SetBodyData(sphere.Body, nullptr, data.Friction, data.Restitution);

      return 1;
    }

    static size_t SphereAndSphere(
      const CollisionSphere& one,
      const CollisionSphere& two,
      CollisionData& data)
    {
      // Cache the sphere positions.
      Vector positionOne = one.GetAxis(3);
      Vector positionTwo = two.GetAxis(3);

      // Find the vector between the objects.
      Vector midline = positionOne - positionTwo;
      float size = midline.Length();

      // See if it is large enough.
      if (size <= 0.0f || size >= one.Radius + two.Radius) return 0;

      // We manually create the normal, because we have the size.
      Vector normal = midline * (1.0f / size);

      // Initialize the contact.
      Contact& contact = data.AddContact();
      contact.ContactNormal = normal;
      contact.ContactPoint = positionOne + midline * 0.5f;
      contact.Penetration = one.Radius + two.Radius - size;
      contact.SetBodyData(one.Body, two.Body, data.Friction, data.Restitution);

      return 1;
    }
  };
} // namespace lite