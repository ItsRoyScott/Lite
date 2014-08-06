#pragma once

#include "Contact.hpp"

namespace lite
{
  class ContactResolver
  {
  private: // data

    size_t positionIterationsUsed = 0;
    size_t velocityIterationsUsed = 0;

  public: // data

    float PositionEpsilon = 0.01f;

    size_t PositionIterations = 0;

    float VelocityEpsilon = 0.01f;

    size_t VelocityIterations = 0;

  public: // methods

    void ResolveContacts(aligned_vector<Contact>& contacts, float dt)
    {
      // Make sure we have something to do.
      if (contacts.size() == 0) return;

      // Prepare the contacts for processing.
      for (auto& contact : contacts)
      {
        contact.CalculateInternals(dt);
      }

      // Resolve the interpenetration problems with the contacts.
      AdjustPositions(contacts.data(), contacts.size(), dt);

      // Resolve the velocity problems with the contacts.
      AdjustVelocities(contacts.data(), contacts.size(), dt);
    }

  private: // methods

    void AdjustPositions(Contact* contacts, size_t numContacts, float dt)
    {
      unsigned i, index;
      float3 linearChange[2], angularChange[2];
      float max;
      Vector deltaPosition;

      // iteratively resolve interpenetrations in order of severity.
      for (positionIterationsUsed = 0; positionIterationsUsed < PositionIterations; ++positionIterationsUsed)
      {
        // Find biggest penetration
        max = PositionEpsilon;
        index = numContacts;
        for (i = 0; i < numContacts; i++)
        {
          if (contacts[i].Penetration > max)
          {
            max = contacts[i].Penetration;
            index = i;
          }
        }
        if (index == numContacts) break;

        // Match the awake state at the contact.
        contacts[index].MatchAwakeState();

        // Resolve the penetration.
        contacts[index].ApplyPositionChange(linearChange, angularChange, max);

        // Again this action may have changed the penetration of other
        // bodies, so we update contacts.
        for (i = 0; i < numContacts; i++)
        {
          // Check each body in the contact.
          for (unsigned b = 0; b < 2; b++)
          {
            if (contacts[i].Body[b])
            {
              // Check for a match with each body in the newly resolved contact.
              for (unsigned d = 0; d < 2; d++)
              {
                if (contacts[i].Body[b] == contacts[index].Body[d])
                {
                  deltaPosition = Vector(linearChange[d]) +
                    Vector(angularChange[d]).Cross(contacts[i].RelativeContactPosition[b]);

                  // The sign of the change is positive if we're dealing with 
                  //   the second body in a contact and negative otherwise.
                  //   (because we're subtracting the resolution)
                  contacts[i].Penetration +=
                    deltaPosition.Dot(contacts[i].ContactNormal) * (b ? 1 : -1);
                }
              }
            }
          }
        }
      }
    }

    void AdjustVelocities(Contact* contacts, size_t numContacts, float dt)
    {
      Vector velocityChange[2], rotationChange[2];
      Vector deltaVel;

      // iteratively handle impacts in order of severity.
      for (velocityIterationsUsed = 0; velocityIterationsUsed < VelocityIterations; ++velocityIterationsUsed)
      {
        // Find contact with maximum magnitude of probable velocity change.
        float max = VelocityEpsilon;
        unsigned index = numContacts;
        for (unsigned i = 0; i < numContacts; i++)
        {
          if (contacts[i].DesiredDeltaVelocity > max)
          {
            max = contacts[i].DesiredDeltaVelocity;
            index = i;
          }
        }
        if (index == numContacts) break;

        // Match the awake state at the contact
        contacts[index].MatchAwakeState();

        // Do the resolution on the contact that came out top.
        contacts[index].ApplyVelocityChange(velocityChange, rotationChange);

        // With the change in velocity of the two bodies, the update of
        // contact velocities means that some of the relative closing
        // velocities need recomputing.
        for (unsigned i = 0; i < numContacts; i++)
        {
          // Check each body in the contact:
          for (unsigned b = 0; b < 2; b++)
          {
            if (contacts[i].Body[b])
            {
              // Check for a match with each body in the newly resolved contact:
              for (unsigned d = 0; d < 2; d++)
              {
                if (contacts[i].Body[b] == contacts[index].Body[d])
                {
                  deltaVel = velocityChange[d] +
                    rotationChange[d].Cross(contacts[i].RelativeContactPosition[b]);

                  // The sign of the change is negative if we're dealing
                  // with the second body in a contact.
                  contacts[i].ContactVelocity = contacts[i].ContactVelocity +
                    contacts[i].ContactToWorld.TransformTranspose(deltaVel)
                    * (b ? -1.0f : 1.0f);
                  contacts[i].CalculateDesiredDeltaVelocity(dt);
                }
              }
            }
          }
        }
      }
    }
  };
} // namespace lite