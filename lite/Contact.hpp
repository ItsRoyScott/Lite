#pragma once

#include "aligned_allocator.hpp"
#include "PhysicsRigidBody.hpp"
#include "Vector.hpp"

namespace lite
{
  // Stores contact information relating to a collision.
  class CollisionData
  {
  public: // data

    // Array of all contacts.
    aligned_vector<Contact> Contacts;

    float Friction = 0;
    float Restitution = 0;
    float Tolerance = 0;

  public: // methods

    // Creates a new contact.
    Contact& AddContact()
    {
      Contacts.emplace_back();
      return Contacts.back();
    }

    // Empties the array of contacts and clears all values.
    void Clear()
    {
      *this = CollisionData();
    }
  };

  // A contact represents two bodies in contact. Resolving a
  //  contact removes their interpenetration, and applies sufficient
  //  impulse to keep them apart.Colliding bodies may also rebound.
  //  Contacts can be used to represent positional joints, by making
  //  the contact constraint keep the bodies in their correct
  //  orientation.
  // 
  // It can be a good idea to create a contact object even when the
  //  contact isn't violated. Because resolving one contact can violate
  //  another, contacts that are close to being violated should be
  //  sent to the resolver; that way if one resolution moves the body,
  //  the contact may be violated, and can be resolved. If the contact
  //  is not violated, it will not be resolved, so you only loose a
  //  small amount of execution time.
  class Contact
  {
  public: // data

    // The two bodies in contact.
    PhysicsRigidBody* Body[2];

    // Direction of the contact in world coordinates.
    Vector ContactNormal;

    // Position of the contact in world coordinates.
    Vector ContactPoint;

    // Matrix converting contact-space to world-space.
    Matrix ContactToWorld;

    // The closing velocity at the point of contact.
    Vector ContactVelocity;

    // Required change in velocity for this contact to be resolved.
    float DesiredDeltaVelocity;

    // The lateral friction coefficient at this contact.
    float Friction;

    // The depth of penetration at the contact point. If both bodies
    //  are specified, then the contact point should be midway
    //  between the interpenetrating points.
    float Penetration;

    // The world space position of the contact point relative to the center of
    //  each body. This is set when the CalculateInternals function is run.
    Vector RelativeContactPosition[2];

    // Normal restitution coefficient at this contact.
    float Restitution;

  public: // methods

    Contact()
    {
      Body[0] = nullptr;
      Body[1] = nullptr;
    }

    // Performs an inertia-weighted penetration resolution of this contact alone.
    void ApplyPositionChange(float3 linearChange[2], float3 angularChange[2], float penetration)
    {
      static const float angularLimit = 0.2f;
      float angularMove[2];
      float linearMove[2];

      float totalInertia = 0;
      float linearInertia[2];
      float angularInertia[2];

      // We need to work out the inertia of each object in the direction
      // of the contact normal, due to angular inertia only.
      for (unsigned i = 0; i < 2; i++)
      {
        if (Body[i])
        {
          const float4x4& inverseInertiaTensor = Body[i]->InverseInertiaTensorWorld();

          // Use the same procedure as for calculating frictionless
          // velocity change to work out the angular inertia.
          Vector angularInertiaWorld = RelativeContactPosition[i].Cross(ContactNormal);
          angularInertiaWorld = Matrix(inverseInertiaTensor).Transform(angularInertiaWorld);
          angularInertiaWorld = angularInertiaWorld.Cross(RelativeContactPosition[i]);
          angularInertia[i] = angularInertiaWorld.Dot(ContactNormal);

          // The linear component is simply the inverse mass
          linearInertia[i] = Body[i]->InverseMass();

          // Keep track of the total inertia from all components
          totalInertia += linearInertia[i] + angularInertia[i];

          // We break the loop here so that the totalInertia value is
          // completely calculated (by both iterations) before
          // continuing.
        }
      }

      // Loop through again calculating and applying the changes.
      for (unsigned i = 0; i < 2; i++)
      {
        if (Body[i])
        {
          // The linear and angular movements required are in proportion to
          // the two inverse inertias.
          float sign = (i == 0) ? 1.0f : -1.0f;
          angularMove[i] = sign * penetration * (angularInertia[i] / totalInertia);
          linearMove[i] = sign * penetration * (linearInertia[i] / totalInertia);

          // To avoid angular projections that are too great (when mass is large
          // but inertia tensor is small) limit the angular move.
          Vector projection = RelativeContactPosition[i];
          projection.AddScaled(ContactNormal, (-RelativeContactPosition[i]).Dot(ContactNormal));

          // Use the small angle approximation for the sine of the angle (i.e.
          // the magnitude would be sine(angularLimit) * projection.magnitude
          // but we approximate sine(angularLimit) to angularLimit).
          float maxMagnitude = angularLimit * projection.Length();
          if (angularMove[i] < -maxMagnitude)
          {
            float totalMove = angularMove[i] + linearMove[i];
            angularMove[i] = -maxMagnitude;
            linearMove[i] = totalMove - angularMove[i];
          }
          else if (angularMove[i] > maxMagnitude)
          {
            float totalMove = angularMove[i] + linearMove[i];
            angularMove[i] = maxMagnitude;
            linearMove[i] = totalMove - angularMove[i];
          }

          // We have the linear amount of movement required by turning
          // the rigid body (in angularMove[i]). We now need to
          // calculate the desired rotation to achieve that.
          if (angularMove[i] == 0)
          {
            // Easy case - no angular movement means no rotation.
            angularChange[i] = { 0, 0, 0 };
          }
          else
          {
            // Work out the direction we'd like to rotate in.
            Vector targetAngularDirection = RelativeContactPosition[i].Cross(ContactNormal);

            const float4x4& inverseInertiaTensor = Body[i]->InverseInertiaTensorWorld();

            // Work out the direction we'd need to rotate to achieve that
            angularChange[i] =
              Vector(Matrix(inverseInertiaTensor).Transform(targetAngularDirection)) *
              (angularMove[i] / angularInertia[i]);
          }

          // Velocity change is easier - it is just the linear movement
          // along the contact normal.
          linearChange[i] = ContactNormal * linearMove[i];

          // Now we can start to apply the values we've calculated.
          // Apply the linear movement.
          Vector pos = Body[i]->Position();
          pos.AddScaled(ContactNormal, linearMove[i]);
          Body[i]->SetPosition(pos);

          // And the change in orientation
          float4 q = Body[i]->Orientation();
          AddScaled(q, angularChange[i], 1.0f);
          Body[i]->SetOrientation(q);

          // We need to calculate the derived data for any body that is
          // asleep, so that the changes are reflected in the object's
          // data. Otherwise the resolution will not change the position
          // of the object, and the next collision detection round will
          // have the same penetration.
          if (!Body[i]->IsAwake())
          {
            Body[i]->CalculateDerivedData();
          }
        }
      }
    }

    // Performs an inertia-weighted impulse based resolution of this contact alone.
    void ApplyVelocityChange(Vector velocityChange[2], Vector rotationChange[2])
    {
      // Get hold of the inverse mass and inverse inertia tensor, both in
      // world coordinates.
      Matrix inverseInertiaTensor[2];
      inverseInertiaTensor[0] = Body[0]->InverseInertiaTensorWorld();
      if (Body[1])
      {
        inverseInertiaTensor[1] = Body[1]->InverseInertiaTensorWorld();
      }

      // We will calculate the impulse for each contact axis
      Vector impulseContact;

      if (Friction == 0.0f)
      {
        // Use the short format for frictionless contacts
        impulseContact = CalculateFrictionlessImpulse(inverseInertiaTensor);
      }
      else
      {
        // Otherwise we may have impulses that aren't in the direction of the
        // contact, so we need the more complex version.
        impulseContact = CalculateFrictionImpulse(inverseInertiaTensor);
      }

      // Convert impulse to world coordinates
      Vector impulse = Matrix(ContactToWorld).Transform(impulseContact);

      // Split in the impulse into linear and rotational components
      Vector impulsiveTorque = RelativeContactPosition[0].Cross(impulse);
      rotationChange[0] = Matrix(inverseInertiaTensor[0]).Transform(impulsiveTorque);
      velocityChange[0] = Vector();
      velocityChange[0] = velocityChange[0].AddScaled(impulse, Body[0]->InverseMass());

      // Apply the changes
      Body[0]->AddVelocity(velocityChange[0]);
      Body[0]->AddRotation(rotationChange[0]);

      if (Body[1])
      {
        // Work out body one's linear and angular changes
        Vector impulsiveTorque = impulse.Cross(RelativeContactPosition[1]);
        rotationChange[1] = Matrix(inverseInertiaTensor[1]).Transform(impulsiveTorque);
        velocityChange[1] = Vector();
        velocityChange[1] = velocityChange[1].AddScaled(impulse, -Body[1]->InverseMass());

        // And apply them.
        Body[1]->AddVelocity(velocityChange[1]);
        Body[1]->AddRotation(rotationChange[1]);
      }
    }

    // Calculates and sets the internal value for the desired delta velocity.
    void CalculateDesiredDeltaVelocity(float dt)
    {
      static const float velocityLimit = 0.25f;

      // Calculate the acceleration induced velocity accumulated this frame.
      float velocityFromAcc = 0;
      if (Body[0]->IsAwake())
      {
        velocityFromAcc += (Body[0]->LastFrameAcceleration() * dt).Dot(ContactNormal);
      }
      if (Body[1] && Body[1]->IsAwake())
      {
        velocityFromAcc -= (Body[1]->LastFrameAcceleration() * dt).Dot(ContactNormal);
      }

      // If the velocity is very slow, limit the restitution.
      float thisRestitution = Restitution;
      float contactVelX = ContactVelocity.GetX();
      if (abs(contactVelX) < velocityLimit)
      {
        thisRestitution = 0.0f;
      }

      // Combine the bounce velocity with the removed
      // acceleration velocity.
      DesiredDeltaVelocity = -contactVelX - thisRestitution * (contactVelX - velocityFromAcc);
    }

    // Calculates internal data from state data. 
    void CalculateInternals(float dt)
    {
      // Check if the first object is null, and swap if it is.
      if (!Body[0])
      {
        SwapBodies();
        FatalIf(!Body[0], "Both bodies null on a contact");
      }

      // Calculate a set of axes at the contact point.
      CalculateContactBasis();

      // Store the relative position of the contact relative to each body.
      RelativeContactPosition[0] = Vector(ContactPoint) - Body[0]->Position();
      if (Body[1]) 
      {
        RelativeContactPosition[1] = Vector(ContactPoint) - Body[1]->Position();
      }

      // Find the relative velocity of the bodies at the contact point.
      ContactVelocity = CalculateLocalVelocity(0, dt);
      if (Body[1]) 
      {
        ContactVelocity = Vector(ContactVelocity) - CalculateLocalVelocity(1, dt);
      }

      // Calculate the desired change in velocity for resolution.
      CalculateDesiredDeltaVelocity(dt);
    }

    // Updates the awake state of rigid bodies that are taking place in the 
    //  given contact. A body will be made awake if it is in contact with a
    //  body that is awake.
    void MatchAwakeState()
    {
      // Collisions with the world never cause a body to wake up.
      if (!Body[1]) return;

      bool body0Awake = Body[0]->IsAwake();
      bool body1Awake = Body[1]->IsAwake();

      // Wake up only the sleeping one.
      if (body0Awake ^ body1Awake) // (XOR is true when only one of the operands is true)
      {
        if (body0Awake) Body[1]->SetAwake(true);
        else Body[0]->SetAwake(true);
      }
    }

    // Sets the data that doesn't normally depend on the position of the contact.
    void SetBodyData(PhysicsRigidBody* one, PhysicsRigidBody* two, float friction, float restitution)
    {
      Body[0] = one;
      Body[1] = two;
      Friction = friction;
      Restitution = restitution;
    }

  private: // methods

    // Calculates an orthonormal basis for the contact point, based on the
    //  primary friction direction (for anisotropic friction) or a random
    //  orientation (for isotropic friction).
    void CalculateContactBasis()
    {
      float3 contactTangent[2];

      // Check whether the Z-axis is nearer to the X or Y axis
      if (abs(ContactNormal.GetX()) > abs(ContactNormal.GetY()))
      {
        // Scaling factor to ensure the results are normalised
        const float s = 1.0f / sqrt(ContactNormal.GetZ()*ContactNormal.GetZ() + ContactNormal.GetX()*ContactNormal.GetX());

        // The new X-axis is at right angles to the world Y-axis
        contactTangent[0].x = ContactNormal.GetZ()*s;
        contactTangent[0].y = 0;
        contactTangent[0].z = -ContactNormal.GetX()*s;

        // The new Y-axis is at right angles to the new X- and Z- axes
        contactTangent[1].x = ContactNormal.GetY()*contactTangent[0].x;
        contactTangent[1].y = ContactNormal.GetZ()*contactTangent[0].x - ContactNormal.GetX()*contactTangent[0].z;
        contactTangent[1].z = -ContactNormal.GetY()*contactTangent[0].x;
      }
      else
      {
        // Scaling factor to ensure the results are normalised
        const float s = 1.0f / sqrt(ContactNormal.GetZ()*ContactNormal.GetZ() + ContactNormal.GetY()*ContactNormal.GetY());

        // The new X-axis is at right angles to the world X-axis
        contactTangent[0].x = 0;
        contactTangent[0].y = -ContactNormal.GetZ()*s;
        contactTangent[0].z = ContactNormal.GetY()*s;

        // The new Y-axis is at right angles to the new X- and Z- axes
        contactTangent[1].x = ContactNormal.GetY()*contactTangent[0].z - ContactNormal.GetZ()*contactTangent[0].y;
        contactTangent[1].y = -ContactNormal.GetX()*contactTangent[0].z;
        contactTangent[1].z = ContactNormal.GetX()*contactTangent[0].y;
      }

      // Make a matrix from the three vectors.
      ContactToWorld = Matrix();
      ContactToWorld = Matrix(ContactToWorld).SetComponents(ContactNormal, contactTangent[0], contactTangent[1]);
    }

    // Calculates the impulse needed to resolve this contact, given that the contact
    //  has a non-zero coefficient of friction. A pair of inertia tensors - one for
    //  each contact object - is specified to save calculation time; the calling 
    //  function has access to these anyway.
    float3 CalculateFrictionImpulse(Matrix* inverseInertiaTensor)
    {
      float3 impulseContact;
      float inverseMass = Body[0]->InverseMass();

      // The equivalent of a cross product in matrices is multiplication
      // by a skew symmetric matrix - we build the matrix for converting
      // between linear and angular quantities.
      Matrix impulseToTorque; 
      impulseToTorque.SetSkewSymmetric(RelativeContactPosition[0]);

      // Build the matrix to convert contact impulse to change in velocity
      // in world coordinates.
      float4x4 deltaVelWorld = impulseToTorque;
      deltaVelWorld *= inverseInertiaTensor[0];
      deltaVelWorld *= impulseToTorque;
      deltaVelWorld *= -1;

      // Check if we need to add body two's data
      if (Body[1])
      {
        // Set the cross product matrix
        impulseToTorque.SetSkewSymmetric(RelativeContactPosition[1]);

        // Calculate the velocity change matrix
        float4x4 deltaVelWorld2 = impulseToTorque;
        deltaVelWorld2 *= inverseInertiaTensor[1];
        deltaVelWorld2 *= impulseToTorque;
        deltaVelWorld2 *= -1;

        // Add to the total delta velocity.
        deltaVelWorld += deltaVelWorld2;

        // Add to the inverse mass
        inverseMass += Body[1]->InverseMass();
      }

      // Do a change of basis to convert into contact coordinates.
      Matrix deltaVelocity = ContactToWorld.Transpose();
      deltaVelocity *= deltaVelWorld;
      deltaVelocity *= ContactToWorld;

      // Add in the linear velocity change
      float4x4 fDeltaVelocity = deltaVelocity;
      fDeltaVelocity.m[0][0] += inverseMass;
      fDeltaVelocity.m[1][1] += inverseMass;
      fDeltaVelocity.m[2][2] += inverseMass;
      deltaVelocity = fDeltaVelocity;

      // Invert to get the impulse needed per unit velocity
      Matrix impulseMatrix = deltaVelocity.Inverse().first;

      // Find the target velocities to kill
      auto velKill = float3(DesiredDeltaVelocity, -ContactVelocity.GetY(), -ContactVelocity.GetZ());

      // Find the impulse to kill target velocities
      impulseContact = impulseMatrix.Transform(velKill);

      // Check for exceeding friction
      float planarImpulse = sqrt(
        impulseContact.y*impulseContact.y +
        impulseContact.z*impulseContact.z);
      if (planarImpulse > impulseContact.x * Friction)
      {
        // We need to use dynamic friction
        impulseContact.y /= planarImpulse;
        impulseContact.z /= planarImpulse;

        impulseContact.x = fDeltaVelocity.m[0][0] +
          fDeltaVelocity.m[0][1] * Friction*impulseContact.y +
          fDeltaVelocity.m[0][2] * Friction*impulseContact.z;
        impulseContact.x = DesiredDeltaVelocity / impulseContact.x;
        impulseContact.y *= Friction * impulseContact.x;
        impulseContact.z *= Friction * impulseContact.x;
      }
      return impulseContact;
    }

    // Calculates the impulse needed to resolve this contact, given that the contact
    //  has no friction. A pair of inertia tensors - one for each contact object - 
    //  is specified to save calculation time; the calling function has access to 
    //  these anyway.
    float3 CalculateFrictionlessImpulse(Matrix* inverseInertiaTensor)
    {
      float3 impulseContact;

      // Build a vector that shows the change in velocity in
      // world space for a unit impulse in the direction of the contact
      // normal.
      Vector deltaVelWorld = RelativeContactPosition[0].Cross(ContactNormal);
      deltaVelWorld = inverseInertiaTensor[0].Transform(deltaVelWorld);
      deltaVelWorld = deltaVelWorld.Cross(RelativeContactPosition[0]);

      // Work out the change in velocity in contact coordiantes.
      float deltaVelocity = deltaVelWorld.Dot(ContactNormal);

      // Add the linear component of velocity change
      deltaVelocity += Body[0]->InverseMass();

      // Check if we need to the second body's data
      if (Body[1])
      {
        // Go through the same transformation sequence again
        Vector deltaVelWorld = RelativeContactPosition[1].Cross(ContactNormal);
        deltaVelWorld = inverseInertiaTensor[1].Transform(deltaVelWorld);
        deltaVelWorld = deltaVelWorld.Cross(RelativeContactPosition[1]);

        // Add the change in velocity due to rotation
        deltaVelocity += deltaVelWorld.Dot(ContactNormal);

        // Add the change in velocity due to linear motion
        deltaVelocity += Body[1]->InverseMass();
      }

      // Calculate the required size of the impulse
      impulseContact.x = DesiredDeltaVelocity / deltaVelocity;
      impulseContact.y = 0;
      impulseContact.z = 0;
      return impulseContact;
    }

    // Calculates and returns the velocity of the contact point on the given body.
    Vector CalculateLocalVelocity(size_t bodyIndex, float dt)
    {
      PhysicsRigidBody *thisBody = Body[bodyIndex];

      // Work out the velocity of the contact point.
      Vector velocity = thisBody->AngularVelocity().Cross(RelativeContactPosition[bodyIndex]);
      velocity += thisBody->Velocity();

      // Turn the velocity into contact-coordinates.
      Vector contactVelocity = ContactToWorld.TransformTranspose(velocity);

      // Calculate the ammount of velocity that is due to forces without
      // reactions.
      Vector accVelocity = thisBody->LastFrameAcceleration() * dt;

      // Calculate the velocity in contact-coordinates.
      accVelocity = ContactToWorld.TransformTranspose(accVelocity);

      // We ignore any component of acceleration in the contact normal
      // direction, we are only interested in planar acceleration
      accVelocity.xm = XMVectorSetX(accVelocity.xm, 0);

      // Add the planar velocities - if there's enough friction they will
      // be removed during velocity resolution
      contactVelocity += accVelocity;

      // And return it
      return contactVelocity;
    }

    // Reverses the contact. This involves swapping the two rigid bodies and
    //  reversing the contat normal. The internal values should then be re-
    //  calculated using CalculateInternals (this is not done automatically).
    void SwapBodies()
    {
      ContactNormal = Vector(ContactNormal) * -1;
      swap(Body[0], Body[1]);
    }

  };
} // namespace lite