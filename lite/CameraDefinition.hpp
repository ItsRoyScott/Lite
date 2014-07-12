#pragma once

#include "float4x4.hpp"

namespace lite
{
  // Manages the properties of the scene's camera.
  class CameraDefinition
  {
  private: // data

    float     aspectRatio;
    float     farWindowHeight;
    float     farZ;
    float     fieldOfViewY;
    float3    look              = { 0, 0, 1 };
    float     nearWindowHeight;
    float     nearZ;
    float3    position          = { 0, 0, 0 };
    float4x4  projectionMatrix;
    float3    right             = { 1, 0, 0 };
    float3    up                = { 0, 1, 0 };
    bool      viewDirty         = true;
    float4x4  viewMatrix;
    float4x4  viewProjection;

  public: // properties

    // Width / Height.
    const float& AspectRatio() const { return aspectRatio; }

    // Height of the far-plane.
    const float& FarWindowHeight() const { return farWindowHeight; }

    // Width of the far-plane.
    float FarWindowWidth() const { return aspectRatio * farWindowHeight; }

    // The far Z value for the camera frustum.
    const float& FarZ() const { return farZ; }

    // The field of view with respect to X.
    float FieldOfViewX() const 
    { 
      float halfWidth = 0.5f*NearWindowWidth();
      return 2.0f * atan(halfWidth / nearZ);
    }

    // The field of view with respect to Y.
    const float& FieldOfViewY() const { return fieldOfViewY; }

    // The camera's look vector.
    const float3& Look() const { return look; }

    // Height of the near-plane.
    const float& NearWindowHeight() const { return nearWindowHeight; }

    // Width of the near-plane.
    float NearWindowWidth() const { return aspectRatio * nearWindowHeight; }

    // The near Z value for the camera frustum.
    const float& NearZ() const { return nearZ; }

    // Position of the camera in the world.
    const float3& Position() const { return position; }

    // Projection matrix.
    const float4x4& ProjectionMatrix() const { return projectionMatrix; }

    // Right vector of the camera.
    const float3& Right() const { return right; }

    // Up vector of the camera.
    const float3& Up() const { return up; }

    // View matrix.
    const float4x4& ViewMatrix() 
    {
      if (viewDirty)
      {
        Update();
      }
      return viewMatrix; 
    }

    // View-projection matrix.
    const float4x4& ViewProjectionMatrix() 
    {
      if (viewDirty)
      {
        Update();
      }
      return viewProjection; 
    }

  public: // methods

    CameraDefinition()
    {
      SetLens();
      Update();
    }

    // Moves the camera up or down by 'distance'.
    void Climb(float distance)
    {
      // position += distance*up
      XMVECTOR s = XMVectorReplicate(distance);
      XMVECTOR u = XMLoadFloat3(&up);
      XMVECTOR p = XMLoadFloat3(&position);
      XMStoreFloat3(&position, XMVectorMultiplyAdd(s, u, p));
      viewDirty = true;
    }

    // Pitch the camera up or down by 'angleInRadians'.
    void Pitch(float angleInRadians)
    {
      // Rotate up and look vector about the right vector.
      XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&right), angleInRadians);
      XMStoreFloat3(&up, XMVector3TransformNormal(XMLoadFloat3(&up), R));
      XMStoreFloat3(&look, XMVector3TransformNormal(XMLoadFloat3(&look), R));
      viewDirty = true;
    }

    // Rotates the camera left or right about the /world/ y-axis.
    void RotateY(float angleInRadians)
    {
      // Rotate the basis vectors about the world y-axis.
      XMMATRIX R = XMMatrixRotationY(angleInRadians);
      XMStoreFloat3(&right, XMVector3TransformNormal(XMLoadFloat3(&right), R));
      XMStoreFloat3(&up, XMVector3TransformNormal(XMLoadFloat3(&up), R));
      XMStoreFloat3(&look, XMVector3TransformNormal(XMLoadFloat3(&look), R));
      viewDirty = true;
    }

    // Set the 'lens' of the camera.
    void SetLens(float fovY = XM_PI/6.0f, float aspect = 16.0f/9.0f, float zn = 0.1f, float zf = 1000.0f)
    {
      // Store the properties.
      fieldOfViewY = fovY;
      aspectRatio = aspect;
      nearZ = zn;
      farZ = zf;

      // Calculate the plane heights.
      nearWindowHeight = 2 * nearZ * tan(0.5f*fovY);
      farWindowHeight = 2 * farZ * tan(0.5f*fovY);

      // Update the projection matrix.
      XMMATRIX proj = XMMatrixPerspectiveFovLH(fieldOfViewY, aspectRatio, nearZ, farZ);
      XMStoreFloat4x4(&projectionMatrix, proj);

      // Update the view-projection matrix.
      XMStoreFloat4x4(&viewProjection, XMMatrixMultiply(XMLoadFloat4x4(&viewMatrix), proj));
    }

    // Strafe left or right by 'distance'.
    void Strafe(float distance)
    {
      // position += distance*right
      XMVECTOR s = XMVectorReplicate(distance);
      XMVECTOR r = XMLoadFloat3(&right);
      XMVECTOR p = XMLoadFloat3(&position);
      XMStoreFloat3(&position, XMVectorMultiplyAdd(s, r, p));
      viewDirty = true;
    }

    // Walk forward or back by 'distance'.
    void Walk(float distance)
    {
      // position += distance*look
      XMVECTOR s = XMVectorReplicate(distance);
      XMVECTOR l = XMLoadFloat3(&look);
      XMVECTOR p = XMLoadFloat3(&position);
      XMStoreFloat3(&position, XMVectorMultiplyAdd(s, l, p));
      viewDirty = true;
    }

  private: // methods

    // Updates the view matrix if it needs updating.
    void Update()
    {
      XMVECTOR R = XMLoadFloat3(&right);
      XMVECTOR U = XMLoadFloat3(&up);
      XMVECTOR L = XMLoadFloat3(&look);
      XMVECTOR P = XMLoadFloat3(&position);

      // Make the look vector unit length.
      L = XMVector3Normalize(L);
      XMStoreFloat3(&look, L);

      // Compute a new corrected "up" vector and normalize it.
      U = XMVector3Normalize(XMVector3Cross(L, R));
      XMStoreFloat3(&up, U);

      // Compute a new corrected "right" vector. U and L are 
      //  already orthonormal, so no need to normalize cross.
      R = XMVector3Cross(U, L);
      XMStoreFloat3(&right, R);

      float x = -XMVectorGetX(XMVector3Dot(P, R));
      float y = -XMVectorGetX(XMVector3Dot(P, U));
      float z = -XMVectorGetX(XMVector3Dot(P, L));

      // Fill in the view matrix entries.
      viewMatrix(0, 0) = right.x;
      viewMatrix(1, 0) = right.y;
      viewMatrix(2, 0) = right.z;
      viewMatrix(3, 0) = x;

      viewMatrix(0, 1) = up.x;
      viewMatrix(1, 1) = up.y;
      viewMatrix(2, 1) = up.z;
      viewMatrix(3, 1) = y;

      viewMatrix(0, 2) = look.x;
      viewMatrix(1, 2) = look.y;
      viewMatrix(2, 2) = look.z;
      viewMatrix(3, 2) = z;

      viewMatrix(0, 3) = 0;
      viewMatrix(1, 3) = 0;
      viewMatrix(2, 3) = 0;
      viewMatrix(3, 3) = 1;

      // Update the view-projection matrix.
      XMStoreFloat4x4(
        &viewProjection,
        XMMatrixMultiply(
        XMLoadFloat4x4(&viewMatrix),
        XMLoadFloat4x4(&projectionMatrix)));

      viewDirty = false;
    }
  };
} // namespace lite