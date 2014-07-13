#pragma once

#include "Matrix.hpp"
#include "Vector.hpp"

namespace lite
{
  // Makes working with XMMATRIX less painful.
  class Matrix
  {
  private: // data

    // Buffer for storing an aligned XMMATRIX (must be 16-byte aligned).
    char buffer[sizeof(XMMATRIX) + 12];

  public: // data

    // XMMATRIX pointer aligned to enable support for SSE intrinsics.
    XMMATRIX* xm = align<XMMATRIX>(16, buffer);

  public: // methods

    Matrix()
    {
      *xm = XMMatrixIdentity();
    }

    Matrix(const float4x4& f)
    {
      *xm = XMLoadFloat4x4(&f);
    }

    Matrix(const XMMATRIX& m)
    {
      *xm = m;
    }

    Vector GetAxisVector(size_t i) const
    {
      return xm->r[i];
    }

    pair<Matrix, Vector> Inverse() const
    {
      pair<Matrix, Vector> result;
      result.first = XMMatrixInverse(result.second.xm, *xm);
      return result;
    }

    Matrix& SetComponents(const float3& one, const float3& two, const float3& three)
    {
      float4x4 f;
      f.m[0][0] = one.x;
      f.m[0][1] = one.y;
      f.m[0][2] = one.z;
      f.m[1][0] = two.x;
      f.m[1][1] = two.y;
      f.m[1][2] = two.z;
      f.m[2][0] = three.x;
      f.m[2][1] = three.y;
      f.m[2][2] = three.z;
      *xm = XMLoadFloat4x4(&f);
      return *this;
    }

    Matrix& SetSkewSymmetric(const float3& vector)
    {
      float4x4 f;
      f.m[0][0] = f.m[1][1] = f.m[2][2] = 0;
      f.m[0][1] = -vector.z;
      f.m[0][2] = vector.y;
      f.m[1][0] = vector.z;
      f.m[1][2] = -vector.x;
      f.m[2][0] = -vector.y;
      f.m[2][1] = vector.x;
      *xm = XMLoadFloat4x4(&f);
      return *this;
    }

    Vector Transform(const Vector& vector) const
    {
      return XMVector3Transform(*vector.xm, *xm);
    }

    Matrix operator*(const Matrix& b) const
    {
      return XMMatrixMultiply(*xm, *b.xm);
    }

    operator float4x4() const
    {
      float4x4 f;
      XMStoreFloat4x4(&f, *xm);
      return f;
    }
  };
} // namespace lite