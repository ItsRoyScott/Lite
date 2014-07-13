#pragma once

#include "D3DInclude.hpp"

namespace lite
{
  // Makes working with XMVECTOR less painful.
  class Vector
  {
  private: // data

    // Buffer used to align the XMVECTOR.
    char buffer[sizeof(XMVECTOR) + 12];

  public: // data

    // 16-byte aligned vector of four floats used for SSE computations.
    XMVECTOR* xm = align<XMVECTOR>(16, buffer);

  public: // methods

    // Initializes with all zeroes.
    Vector()
    {
      *xm = XMVectorZero();
    }

    Vector(const Vector&) = default;
    Vector& operator=(const Vector&) = default;

    // Initialize from float3.
    Vector(const float3& f)
    {
      *this = f;
    }

    // Assign a float3.
    Vector& operator=(const float3& f)
    {
      *xm = XMLoadFloat3(&f);
      return *this;
    }

    // Initialize from float4.
    Vector(const float4& f)
    {
      *this = f;
    }

    // Assign a float4.
    Vector& operator=(const float4& f)
    {
      *xm = XMLoadFloat4(&f);
      return *this;
    }

    // Initialize four floats.
    Vector(const float f[4])
    {
      *this = f;
    }

    // Assign four floats.
    Vector& operator=(const float f[4])
    {
      return *this = float4(f);
    }

    // Initialize from XMVECTOR.
    Vector(const XMVECTOR& vec)
    {
      *this = vec;
    }

    // Assign from an XMVECTOR.
    Vector& operator=(const XMVECTOR& vec)
    {
      *xm = vec;
      return *this;
    }

    // Initialize with x, y, z, w components.
    Vector(float x, float y = 0, float z = 0, float w = 0)
    {
      *xm = XMVectorSet(x, y, z, w);
    }

    ~Vector() = default;

    // Multiplies the input vector by the scalar 
    //  then adds the result to this vector.
    Vector& AddScaled(const Vector& vec, float scale)
    {
      return *this += vec * scale;
    }

    // Returns the result of cross product.
    Vector Cross(const Vector& b) const
    {
      return XMVector3Cross(*xm, *b.xm);
    }

    // Returns the result of dot product.
    float Dot(const Vector& b) const
    {
      return XMVectorGetX(XMVector3Dot(*xm, *b.xm));
    }

    float GetX() const
    {
      return XMVectorGetX(*xm);
    }

    float GetY() const
    {
      return XMVectorGetY(*xm);
    }

    float GetZ() const
    {
      return XMVectorGetZ(*xm);
    }

    float GetW() const
    {
      return XMVectorGetW(*xm);
    }

    // Returns the magnitude of the 3-float vector.
    float Length() const
    {
      return XMVectorGetX(XMVector3Length(*xm));
    }

    // Adds the vectors.
    Vector operator+(const Vector& b) const
    {
      return XMVectorAdd(*xm, *b.xm);
    }

    // Add-assigns the vector.
    Vector& operator+=(const Vector& b)
    {
      return *this = *this + b;
    }

    // Multiplies the vector by a scalar.
    Vector operator*(float f) const
    {
      return XMVectorScale(*xm, f);
    }

    // Multiply-assigns the vector by a scalar.
    Vector& operator*=(float f)
    {
      return *this = *this * f;
    }

    // Subtracts the vectors.
    Vector operator-(const Vector& b) const
    {
      return XMVectorSubtract(*xm, *b.xm);
    }

    // Subtract-assigns the vectors.
    Vector operator-=(const Vector& b)
    {
      return *this = *this - b;
    }

    // Unary minus.
    Vector operator-() const
    {
      return XMVectorNegate(*xm);
    }

    // Conversion to float3.
    operator float3() const
    {
      float3 f;
      XMStoreFloat3(&f, *xm);
      return f;
    }

    // Conversion to float4.
    operator float4() const
    {
      float4 f;
      XMStoreFloat4(&f, *xm);
      return f;
    }
  };

  inline float4& AddScaled(float4& f, const float3& vector, float scale)
  {
    Vector q = float4(0, vector.x*scale, vector.y*scale, vector.z*scale);
    *q.xm = XMQuaternionMultiply(*q.xm, *Vector(f).xm);
    f.w += XMVectorGetW(*q.xm) * 0.5f;
    f.x += XMVectorGetX(*q.xm) * 0.5f;
    f.y += XMVectorGetY(*q.xm) * 0.5f;
    f.z += XMVectorGetZ(*q.xm) * 0.5f;
    return f;
  }
} // namespace lite