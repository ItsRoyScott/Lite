#pragma once

#include "D3DInclude.hpp"

namespace lite
{
  class float4x4 : public XMFLOAT4X4
  {
  public: // methods

    float4x4()
    {
      XMStoreFloat4x4(this, XMMatrixIdentity());
    }

    float4x4(const float4x4& b) :
      XMFLOAT4X4(b)
    {}

    float4x4& operator=(const float4x4& b)
    {
      XMFLOAT4X4::operator=(b);
      return *this;
    }

    float4x4(const XMMATRIX& m)
    {
      XMStoreFloat4x4(this, m);
    }

    template <size_t Index>
    float& At()
    {
      return m[Index / 4][Index % 4];
    }

    template <size_t Index>
    float At() const
    {
      return m[Index / 4][Index % 4];
    }

    float& At(size_t index)
    {
      return m[index / 4][index % 4];
    }

    float At(size_t index) const
    {
      return m[index / 4][index % 4];
    }

    float4 GetAxisVector(size_t i) const
    {
      return float4(At(i), At(i + 4), At(i + 8), At(i + 12));
    }

    float4x4 Inverse() const
    {
      float4x4 f;
      XMVECTOR det;
      XMStoreFloat4x4(&f, XMMatrixInverse(&det, XMLoadFloat4x4(this)));
      return f;
    }

    void SetComponents(const float3& one, const float3& two, const float3& three)
    {
      At<0>() = one.x;
      At<1>() = two.x;
      At<2>() = three.x;
      At<3>() = one.y;
      At<4>() = two.y;
      At<5>() = three.y;
      At<6>() = one.z;
      At<7>() = two.z;
      At<8>() = three.z;
    }

    void SetSkewSymmetric(const float3& vector)
    {
      At<0>() = At<4>() = At<8>() = 0;
      At<1>() = -vector.z;
      At<2>() = vector.y;
      At<3>() = vector.z;
      At<5>() = -vector.x;
      At<6>() = -vector.y;
      At<7>() = vector.x;
    }

    float3 Transform(const float3& vector) const
    {
      return float3(
        vector.x * At<0>() + vector.y * At<1>() + vector.z * At<2>(),
        vector.x * At<3>() + vector.y * At<4>() + vector.z * At<5>(),
        vector.x * At<6>() + vector.y * At<7>() + vector.z * At<8>()
        );
    }

    float3 TransformInverse(const float3& vector) const
    {
      float3 tmp = vector;
      tmp.x -= At<3>();
      tmp.y -= At<7>();
      tmp.z -= At<11>();
      return float3(
        tmp.x * At<0>() +
        tmp.y * At<4>() +
        tmp.z * At<8>(),

        tmp.x * At<1>() +
        tmp.y * At<5>() +
        tmp.z * At<9>(),

        tmp.x * At<2>() +
        tmp.y * At<6>() +
        tmp.z * At<10>()
        );
    }

    float3 TransformTranspose(const float3& vector) const
    {
      return float3(
        vector.x * At<0>() + vector.y * At<3>() + vector.z * At<6>(),
        vector.x * At<1>() + vector.y * At<4>() + vector.z * At<7>(),
        vector.x * At<2>() + vector.y * At<5>() + vector.z * At<8>()
        );
    }

    float4x4 Transpose() const
    {
      float4x4 f;
      XMStoreFloat4x4(&f, XMMatrixTranspose(XMLoadFloat4x4(this)));
      return f;
    }

    float4x4& operator+=(const float4x4& o)
    {
      At<0>() += o.At<0>(); At<1>() += o.At<1>(); At<2>() += o.At<2>();
      At<3>() += o.At<3>(); At<4>() += o.At<4>(); At<5>() += o.At<5>();
      At<6>() += o.At<6>(); At<7>() += o.At<7>(); At<8>() += o.At<8>();
      return *this;
    }

    float4x4& operator*=(float scalar)
    {
      At<0>() *= scalar; At<1>() *= scalar; At<2>() *= scalar;
      At<3>() *= scalar; At<4>() *= scalar; At<5>() *= scalar;
      At<6>() *= scalar; At<7>() *= scalar; At<8>() *= scalar;
      return *this;
    }

    float4x4 operator*(const float4x4& b) const
    {
      float4x4 f = *this;
      f *= b;
      return f;
    }

    float4x4& operator*=(const float4x4& b)
    {
      XMMATRIX A = XMLoadFloat4x4(this);
      XMMATRIX B = XMLoadFloat4x4(&b);
      XMStoreFloat4x4(this, XMMatrixMultiply(A, B));
      return *this;
    }
  };
} // namespace lite