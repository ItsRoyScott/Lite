#pragma once

#include "D3DInclude.hpp"
#include "Essentials.hpp"
#include "float4x4.hpp"
#include "GameObject.hpp"

namespace lite
{
  class Transform : public Component<Transform>
  {
  public: // properties

    // Position x, y, and z.
    float3 LocalPosition = { 0, 0, 0 };

    // Rotation as a quaternion.
    float4 LocalRotation = { 0, 0, 0, 1 };

    // Scale factor.
    float3 LocalScale = { 1, 1, 1 };

  public: // methods

    Transform() = default;

    Transform(const Transform& b) :
      LocalPosition(b.LocalPosition),
      LocalRotation(b.LocalRotation),
      LocalScale(b.LocalScale)
    {}

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

    XMMATRIX GetOffsetFromParent(Transform& parent) const
    {
      if (&parent == this)
      {
        return XMMatrixIdentity();
      }

      // Multiply this local transform with the parent's offset.
      if (Owner() && Owner()->Parent())
      {
        return OwnerReference().ParentReference()[Transform_].GetOffsetFromParent(parent) * GetLocalMatrix();
      }

      // No parents: return the local matrix.
      return GetLocalMatrix();
    }

    // Transformation formed by this transform and all of its parents.
    XMMATRIX GetWorldMatrix() const
    {
      // Multiply this local transform with the parent's world transform.
      if (Owner() && Owner()->Parent())
      {
        return OwnerReference().ParentReference()[Transform_].GetWorldMatrix() * GetLocalMatrix();
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
    void SetLocalMatrix(const float4x4& matrix)
    {
      // Decompoose the given matrix.
      XMVECTOR scale, quat, trans;
      XMMatrixDecompose(&scale, &quat, &trans, XMLoadFloat4x4(&matrix));

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

  template<>
  struct Binding<Transform> : BindingBase<Transform>
  {
    Binding()
    {
      Bind(
        "LocalPosition", &T::LocalPosition,
        "LocalRotation", &T::LocalRotation,
        "LocalScale", &T::LocalScale);
    }
  };
} // namespace lite