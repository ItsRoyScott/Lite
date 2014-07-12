#pragma once

#include "D3DInclude.hpp"

namespace lite
{
  // Makes working with XMVECTOR less painful.
  class Vector
  {
  public: // data

    XMVECTOR xm;

  public: // methods

    // Initializes with all zeroes.
    Vector()
    {
      xm = XMVectorZero();
    }

    Vector(const Vector&) = default;

    // Initialize from float3.
    Vector(const float3& f)
    {
      *this = f;
    }

    // Initialize from float4.
    Vector(const float4& f)
    {
      *this = f;
    }

    // Initialize with x, y, z, w components.
    Vector(float x, float y = 0, float z = 0, float w = 0)
    {
      xm = XMVectorSet(x, y, z, w);
    }

    // Initialize from XMVECTOR.
    Vector(const XMVECTOR& vec)
    {
      xm = vec;
    }

    ~Vector() = default;

    Vector& operator=(const Vector&) = default;

    // Assign a float3.
    Vector& operator=(const float3& f)
    {
      xm = XMLoadFloat3(&f);
      return *this;
    }

    // Assign a float4.
    Vector& operator=(const float4& f)
    {
      xm = XMLoadFloat4(&f);
      return *this;
    }

    // Multiplies the input vector by the scalar 
    //  then adds the result to this vector.
    Vector& AddScaled(const Vector& vec, float scale)
    {
      return *this += vec * scale;
    }

    // Returns the result of cross product.
    Vector Cross(const Vector& b) const
    {
      return XMVector3Cross(xm, b.xm);
    }

    // Returns the result of dot product.
    float Dot(const Vector& b) const
    {
      return XMVectorGetX(XMVector3Dot(xm, b.xm));
    }

    // Returns the magnitude of the 3-float vector.
    float Length() const
    {
      return XMVectorGetX(XMVector3Length(xm));
    }

    // Adds the vectors.
    Vector operator+(const Vector& b) const
    {
      return XMVectorAdd(xm, b.xm);
    }

    // Add-assigns the vector.
    Vector& operator+=(const Vector& b)
    {
      return *this = *this + b;
    }

    // Multiplies the vector by a scalar.
    Vector operator*(float f) const
    {
      return XMVectorScale(xm, f);
    }

    // Multiply-assigns the vector by a scalar.
    Vector& operator*=(float f)
    {
      return *this = *this * f;
    }

    // Subtracts the vectors.
    Vector operator-(const Vector& b) const
    {
      return XMVectorSubtract(xm, b.xm);
    }

    // Subtract-assigns the vectors.
    Vector operator-=(const Vector& b)
    {
      return *this = *this - b;
    }

    // Unary minus.
    Vector operator-() const
    {
      return XMVectorNegate(xm);
    }

    // Conversion to float3.
    operator float3() const
    {
      float3 f;
      XMStoreFloat3(&f, xm);
      return f;
    }

    // Conversion to float4.
    operator float4() const
    {
      float4 f;
      XMStoreFloat4(&f, xm);
      return f;
    }
  };

  inline float4& AddScaled(float4& f, const float3& vector, float scale)
  {
    Vector q = float4(0, vector.x*scale, vector.y*scale, vector.z*scale);
    q.xm = XMQuaternionMultiply(q.xm, Vector(f).xm);
    f.w += XMVectorGetW(q.xm) * 0.5f;
    f.x += XMVectorGetX(q.xm) * 0.5f;
    f.y += XMVectorGetY(q.xm) * 0.5f;
    f.z += XMVectorGetZ(q.xm) * 0.5f;
    return f;
  }
} // namespace lite