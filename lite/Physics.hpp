#pragma once

#include "D3DInclude.hpp"
#include "Essentials.hpp"

namespace lite
{
  class Particle
  {
  private: // data

    float3 acceleration = { 0, 0, 0 };
    float3 accumulatedForces = { 0, 0, 0 };
    float inverseMass = 1;
    float mass = 1;
    float3 position = { 0, 0, 0 };
    float3 velocity = { 0, 0, 0 };

  public: // data

    // Functions which add in a force or otherwise act on the particle.
    vector<function<void(Particle& particle, float dt)>> Actors;

    // Damping used to make velocity integration more realistic.
    float Damping = 0.999f;

  public: // properties

    // Acceleration in meters per second squared.
    const float3& Acceleration() const { return acceleration; }

    // Summation of forces currently acting on this object.
    const float3& AccumulatedForces() const { return accumulatedForces; }

    // 1 / Mass.
    const float& InverseMass() const { return inverseMass; }

    // Mass in kilograms.
    const float& Mass() const { return mass; }

    // Position in meters.
    const float3& Position() const { return position; }

    // Velocity in meters per second.
    const float3& Velocity() const { return velocity; }

  public:

    void AddForce(float3 vector)
    {
      XMVECTOR newForces = XMVectorAdd(XMLoadFloat3(&accumulatedForces), XMLoadFloat3(&vector));
      XMStoreFloat3(&accumulatedForces, newForces);
    }

    void Integrate(float dt)
    {
      // Early out for zero-mass objects.
      if (inverseMass <= 0) return;

      // Call on all actors.
      for (auto& actor : Actors)
      {
        actor(*this, dt);
      }

      // Update position based on the current velocity.
      XMVECTOR vt = XMVectorScale(XMLoadFloat3(&velocity), dt);
      XMVECTOR newPosition = XMVectorAdd(XMLoadFloat3(&position), vt);

      // Update acceleration based on the current forces acting on this object.
      XMVECTOR fm = XMVectorScale(XMLoadFloat3(&accumulatedForces), inverseMass);
      XMVECTOR newAcceleration = XMVectorAdd(XMLoadFloat3(&acceleration), fm);

      // Update velocity based on the current acceleration of the object.
      XMVECTOR at = XMVectorScale(newAcceleration, dt);
      XMVECTOR newVelocity = XMVectorAdd(XMLoadFloat3(&velocity), at);

      // Damp velocity.
      newVelocity = XMVectorScale(newVelocity, pow(Damping, dt));

      // Reset forces applied.
      acceleration = 0;
    }

    void SetMass(float m)
    {
      inverseMass = 1.0f / m;
      mass = m;
    }
  };

  class Physics
  {
  private: // data

    bool addDefaultGravity = true;
    function<void(Particle&, float)> defaultGravityActor;
    vector<unique_ptr<Particle>> particles;

  public: // methods

    Physics(bool addGravity = true, float3 defaultGravityVector = { 0, -9.8f, 0 }) :
      addDefaultGravity(addGravity)
    {
      defaultGravityActor = [=](Particle& p, float) { p.AddForce(defaultGravityVector); };
    }

    Particle& AddParticle()
    {
      // Create the new particle.
      particles.push_back(make_unique<Particle>());
      Particle& particle = *particles.back();

      // Add default gravity actor if it was requested at startup.
      if (addDefaultGravity)
      {
        particle.Actors.push_back(defaultGravityActor);
      }

      return particle;
    }

    void Update(float dt)
    {
      for (auto& particle : particles)
      {
        particle->Integrate(dt);
      }
    }
  };
} // namespace lite