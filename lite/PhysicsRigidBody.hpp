#pragma once

#include "PhysicsUtility.hpp"

namespace lite
{
  class PhysicsRigidBody
  {
  private: // data

    // Current acceleration of the rigid body.
    float3 acceleration = { 0, 0, 0 };

    // Forces accumulated on the rigid body.
    float3 accumulatedForces = { 0, 0, 0 };

    // Accumulated torque to be applied at the next integration step.
    float3 accumulatedTorque = { 0, 0, 0 };

    // Amount that the rigid body is rotating in world space.
    float3 angularVelocity = { 0, 0, 0 };

    // Inverse of the body's inertia tensor. The inertia tensor provided must not
    //  degenerate (that would mean the body had zero inertia for spinning along
    //  one axis). As long as the tensor is finite, it will be invertible. The
    //  inverse tenssor is used for similar reasons to the use of inverse mass.
    float4x4 inverseInertiaTensor;

    // Holds the inverse inertia tensor of the body in world space. The inverse
    //  inertia tensor member is specified in the body's local space.
    float4x4 inverseInertiaTensorWorld;

    // Holds the inverse of the mass of the rigid body. It is more useful to hold the
    //  inverse mass because integration is simpler, and because in real-time
    //  simulation it is more useful to have bodies with infinite mass (immovable)
    //  than zero mass (completely unstable in numerical simulation).
    float inverseMass = 1;

    // A body can be put to sleep to avoid it being updated by the integration
    //  functions or affected by collisions with the world.
    bool isAwake = false;

    // Linear acceleration of the rigid body for the previous frame.
    float3 lastFrameAcceleration = { 0, 0, 0 };

    // The amount of motion of the body. This is a recency weighted mean that
    //  can be used to put a body to sleep.
    float motion = 0;

    // Angular orientation of the rigid body in world space.
    float4 orientation = { 0, 0, 0, 1 };

    // Position of the rigid body in world space.
    float3 position = { 0, 0, 0 };

    // Used for converting from local to world space and back.
    float4x4 transformMatrix;

    // Velocity of the rigid body in world space.
    float3 velocity = { 0, 0, 0 };

  public: // data

    // Functions which add in a force or otherwise act on the rigid body.
    vector<function<void(PhysicsRigidBody& body, float dt)>> Actors;

    // The amount of damping applied to angular motion. Damping is required
    //  to remove energy added through numerical instability.
    float AngularDamping = 0.8f;

    // Some bodies may never be allowed to fall asleep. User-controlled bodies,
    //  for example, should be always awake.
    bool CanSleep = true;

    // The amount of damping applied to linear motion. Damping is required to
    //  remove energy added through numerical instability.
    float LinearDamping = 0.999f;

  public: // properties

    // Acceleration in meters per second squared.
    Vector Acceleration() const { return acceleration; }

    // Summation of forces currently acting on this object.
    Vector AccumulatedForces() const { return accumulatedForces; }

    // Amount that the rigid body is rotating in world space.
    Vector AngularVelocity() const { return angularVelocity; }

    const float4x4& InverseInertiaTensorWorld() const { return inverseInertiaTensorWorld; }

    // 1 / Mass.
    const float& InverseMass() const { return inverseMass; }

    const bool& IsAwake() const { return isAwake; }

    // Linear acceleration of the rigid body for the previous frame.
    Vector LastFrameAcceleration() const { return lastFrameAcceleration; }

    // Mass in kilograms.
    float Mass() const { return inverseMass == 0 ? numeric_limits<float>::max() : 1.0f / inverseMass; }

    Vector Orientation() const { return orientation; }

    // Position in meters.
    Vector Position() const { return position; }

    // Orientation in world space.
    const float4x4& Transform() const { return transformMatrix; }

    // Velocity in meters per second.
    Vector Velocity() const { return velocity; }

  public:

    void AddForce(float3 vector)
    {
      if (!HasFiniteMass()) return;

      accumulatedForces = AccumulatedForces() + vector;
      isAwake = true;
    }

    // Adds the given force to the given point on the rigid body. The direction
    //  of the force is given in world coordinates, but the application point
    //  is given in body space. This is useful for spring forces, or other
    //  forces fixed on the body.
    void AddForceAtBodyPoint(const float3& force, const float3& point)
    {
      if (!HasFiniteMass()) return;

      // Convert to coordinates relative to center of mass.
      float3 pt = GetPointInWorldSpace(point);
      AddForceAtPoint(force, pt);
    }

    // Adds the given force to the given point on the rigid body. Both the
    //  force and the application point are given in world space. Because
    //  the force is not applied at the center of mass, it may be split
    //  into both a force and torque.
    void AddForceAtPoint(const float3& force, const float3& point)
    {
      if (!HasFiniteMass()) return;

      // Convert to coordinates relative to center of mass.
      Vector pt = point;
      pt -= position;

      accumulatedForces = AccumulatedForces() + force;
      accumulatedTorque = pt.Cross(force);

      isAwake = true;
    }

    // Converts the given point from world space into the body's local space.
    float3 GetPointInLocalSpace(const float3& worldPoint) const
    {
      return transformMatrix.TransformInverse(worldPoint);
    }

    // Converts the given point from the body's local space to world space.
    float3 GetPointInWorldSpace(const float3& localPoint) const
    {
      return transformMatrix.Transform(localPoint);
    }

    // True if the mass of the body is not infinite.
    bool HasFiniteMass() const
    {
      return inverseMass >= 0.0f;
    }

    void SetMass(float m)
    {
      if (m <= 0)
      {
        inverseMass = -1;
      }
      else
      {
        inverseMass = 1.0f / m;
      }
    }

  private: // methods

    friend class Contact;
    friend class ParticleContact;
    friend class Physics;
    friend class World;

    void AddRotation(const float3& deltaRotation)
    {
      angularVelocity = AngularVelocity() + deltaRotation;
    }

    void AddVelocity(const float3& deltaVelocity)
    {
      velocity = Velocity() + deltaVelocity;
    }

    void ApplyActors(float dt)
    {
      for (auto& actor : Actors)
      {
        actor(*this, dt);
      }
    }

    // Calculates internal data from state data. This should be called after the
    //  body's state is altered directly (it is called automatically during
    //  integration). If you change the body's state and then intend to 
    //  integrate before querying any data (such as the transform matrix), then
    //  you can omit this step.
    void CalculateDerivedData()
    {
      Vector orientationQuaternion;
      orientationQuaternion = XMQuaternionNormalize(orientationQuaternion.xm);

      // Calculate the transform matrix for the body.
      Matrix rotation = XMMatrixRotationQuaternion(orientationQuaternion.xm);
      Matrix translation = XMMatrixTranslationFromVector(Vector(position).xm);
      transformMatrix = rotation * translation;

      // Calculate the inertia tensor in world space.
      TransformInertiaTensor(
        inverseInertiaTensorWorld, 
        orientation, 
        inverseInertiaTensor, 
        transformMatrix);
    }

    void ClearAccumulators()
    {
      accumulatedForces = { 0, 0, 0 };
      accumulatedTorque = { 0, 0, 0 };
    }

    void Integrate(float dt)
    {
      // Calculate linear acceleration from force inputs.
      lastFrameAcceleration = acceleration;
      lastFrameAcceleration = Vector(lastFrameAcceleration).AddScaled(accumulatedForces, inverseMass);

      // Calculate angular acceleration from torque inputs.
      float3 angularAcceleration = inverseInertiaTensorWorld.Transform(accumulatedTorque);

      // Update linear velocity from both acceleration and impulse.
      velocity = Vector(velocity).AddScaled(lastFrameAcceleration, dt);

      // Update angular velocity from both acceleration and impulse.
      angularVelocity = Vector(angularVelocity).AddScaled(angularAcceleration, dt);

      // Impose drag.
      velocity = Vector(velocity) * pow(LinearDamping, dt);
      angularVelocity = Vector(angularAcceleration) * pow(AngularDamping, dt);

      // Update position based on the current velocity.
      position = Vector(position).AddScaled(Velocity(), dt);

      // Update angular position.
      orientation = Vector(orientation).AddScaled(angularVelocity, dt);

      // Normalize the orientation, and update the matrices with the new 
      //  position and orientation.
      CalculateDerivedData();

      // Reset forces applied.
      ClearAccumulators();
    }

    void SetAwake(bool awake)
    {
      if (awake)
      {
        isAwake = true;

        // Add a bit of motion to avoid it falling asleep immediately.
        motion = SleepEpsilon * 2.0f;
      }
      else
      {
        isAwake = false;
        velocity = { 0, 0, 0 };
        angularVelocity = { 0, 0, 0 };
      }
    }

    void SetInertiaTensor(Matrix& m)
    {
      inverseInertiaTensor = m.Inverse().first;
    }

    void SetOrientation(const Vector& q)
    {
      XMStoreFloat4(&orientation, XMQuaternionNormalize(q.xm));
    }

    void SetPosition(const float3& position)
    {
      this->position = position;
    }

    // Internal function to do an inertia tensor transform by a quaternion.
    static void TransformInertiaTensor(
      float4x4& iitWorld,
      const float4& q,
      const float4x4& iitBody,
      const float4x4& rotmat)
    {
      float t4 = rotmat.At<0>() * iitBody.At<0>() +
        rotmat.At<1>() * iitBody.At<3>() +
        rotmat.At<2>() * iitBody.At<6>();
      float t9 = rotmat.At<0>() * iitBody.At<1>() +
        rotmat.At<1>() * iitBody.At<4>() +
        rotmat.At<2>() * iitBody.At<7>();
      float t14 = rotmat.At<0>() * iitBody.At<2>() +
        rotmat.At<1>() * iitBody.At<5>() +
        rotmat.At<2>() * iitBody.At<8>();
      float t28 = rotmat.At<4>() * iitBody.At<0>() +
        rotmat.At<5>() * iitBody.At<3>() +
        rotmat.At<6>() * iitBody.At<6>();
      float t33 = rotmat.At<4>() * iitBody.At<1>() +
        rotmat.At<5>() * iitBody.At<4>() +
        rotmat.At<6>() * iitBody.At<7>();
      float t38 = rotmat.At<4>() * iitBody.At<2>() +
        rotmat.At<5>() * iitBody.At<5>() +
        rotmat.At<6>() * iitBody.At<8>();
      float t52 = rotmat.At<8>() * iitBody.At<0>() +
        rotmat.At<9>() * iitBody.At<3>() +
        rotmat.At<10>() * iitBody.At<6>();
      float t57 = rotmat.At<8>() * iitBody.At<1>() +
        rotmat.At<9>() * iitBody.At<4>() +
        rotmat.At<10>() * iitBody.At<7>();
      float t62 = rotmat.At<8>() * iitBody.At<2>() +
        rotmat.At<9>() * iitBody.At<5>() +
        rotmat.At<10>() * iitBody.At<8>();

      iitWorld.At<0>() = t4*rotmat.At<0>() +
        t9*rotmat.At<1>() +
        t14*rotmat.At<2>();
      iitWorld.At<1>() = t4*rotmat.At<4>() +
        t9*rotmat.At<5>() +
        t14*rotmat.At<6>();
      iitWorld.At<2>() = t4*rotmat.At<8>() +
        t9*rotmat.At<9>() +
        t14*rotmat.At<10>();
      iitWorld.At<3>() = t28*rotmat.At<0>() +
        t33*rotmat.At<1>() +
        t38*rotmat.At<2>();
      iitWorld.At<4>() = t28*rotmat.At<4>() +
        t33*rotmat.At<5>() +
        t38*rotmat.At<6>();
      iitWorld.At<5>() = t28*rotmat.At<8>() +
        t33*rotmat.At<9>() +
        t38*rotmat.At<10>();
      iitWorld.At<6>() = t52*rotmat.At<0>() +
        t57*rotmat.At<1>() +
        t62*rotmat.At<2>();
      iitWorld.At<7>() = t52*rotmat.At<4>() +
        t57*rotmat.At<5>() +
        t62*rotmat.At<6>();
      iitWorld.At<8>() = t52*rotmat.At<8>() +
        t57*rotmat.At<9>() +
        t62*rotmat.At<10>();
    }
  };
} // namespace lite