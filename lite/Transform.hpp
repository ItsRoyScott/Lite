#pragma once

#include "D3DInclude.hpp"
#include "Essentials.hpp"

namespace lite
{
  class Transform
  {
  public: // properties

    // Position x, y, and z.
    float3 LocalPosition = { 0, 0, 0 };

    // Rotation as a quaternion.
    float4 LocalRotation = { 1, 0, 0, 0 };

    // Scale factor.
    float3 LocalScale = { 1, 1, 1 };

  public: // methods

    // Transformation formed by this transform only (doesn't include parents).
    XMMATRIX GetLocalMatrix() const
    {
      return XMMatrixTransformation(
        XMVectorZero(),                 // center (position) of scaling
        XMVectorZero(),                 // orientation (rotation) of the scaling
        XMLoadFloat3(&LocalScale),      // scaling factors
        XMVectorZero(),                 // center (position) of rotation
        XMLoadFloat4(&LocalRotation),   // quaternion rotation
        XMLoadFloat3(&LocalPosition));  // translation
    }

    // Transformation formed by this transform and all of its parents.
    XMMATRIX GetWorldMatrix() const
    {
      // Multiply this local transform with the parent's world transform.
      if (Parent)
      {
        return GetLocalMatrix() * Parent->GetWorldMatrix();
      }

      // No parents: return the local matrix.
      return GetLocalMatrix();
    }

    // Rotate roll (z), then pitch (x), then yaw (y).
    void RotateBy(float3 eulerAngles)
    {
      // Form a quaternion which represents the desired rotation.
      XMVECTOR rot = XMQuaternionRotationRollPitchYaw(
        eulerAngles.x,
        eulerAngles.y,
        eulerAngles.z);

      // Multiply with the current rotation and store in LocalRotation.
      XMStoreFloat4(
        &LocalRotation, 
        XMQuaternionMultiply(
          XMLoadFloat4(&LocalRotation), 
          rot));
    }

    // Multiplies x, y, z components to the current scale.
    void ScaleBy(float3 scaleFactor)
    {
      XMStoreFloat3(
        &LocalScale, 
        XMVectorMultiply(
          XMLoadFloat3(&LocalScale), 
          XMLoadFloat3(&scaleFactor)));
    }

    // Sets local properties to construct the matrix.
    void SetLocalMatrix(XMMATRIX matrix)
    {
      // Decompoose the given matrix.
      XMVECTOR scale, quat, trans;
      XMMatrixDecompose(&scale, &quat, &trans, matrix);

      // Save the results.
      XMStoreFloat3(&LocalScale, scale);
      XMStoreFloat4(&LocalRotation, quat);
      XMStoreFloat3(&LocalPosition, trans);
    }

    // Offset position x, y, z.
    void TranslateBy(float3 positionOffset)
    {
      XMStoreFloat3(
        &LocalPosition, 
        XMVectorAdd(
          XMLoadFloat3(&LocalPosition), 
          XMLoadFloat3(&positionOffset)));
    }
  };
} // namespace lite