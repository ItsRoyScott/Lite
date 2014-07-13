#pragma once

#include "D3DInclude.hpp"
#include "Vector.hpp"

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
      return m[Index % 4][Index / 4];
    }

    template <size_t Index>
    float At() const
    {
      return m[Index % 4][Index / 4];
    }

    float& At(size_t index)
    {
      return m[index % 4][index / 4];
    }

    float At(size_t index) const
    {
      return m[index % 4][index / 4];
    }

    // data[0] -> m[0][0]
    // data[1] -> m[0][1]
    // data[2] -> m[0][2]
    // data[3] -> m[1][0]
    // data[4] -> m[1][1]
    // data[5] -> m[1][2]
    // data[6] -> m[2][0]
    // data[7] -> m[2][1]
    // data[8] -> m[2][2]

    //Vector Transform(const Vector& vector) const
    //{
    //  return XMVector3Transform(vector.xm, XMLoadFloat4x4(this));
    //}

    Vector TransformInverse(const Vector& vector) const
    {
      XMVECTOR det;
      return XMVector3Transform(*vector.xm, XMMatrixInverse(&det, XMLoadFloat4x4(this)));
    }

    Vector TransformTranspose(const Vector& vector) const
    {
      return XMVector3Transform(*vector.xm, XMMatrixTranspose(XMLoadFloat4x4(this)));
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
      At<9>() += o.At<9>(); At<10>() += o.At<10>(); At<11>() += o.At<11>();
      return *this;
    }

    float4x4& operator*=(float scalar)
    {
      At<0>() *= scalar; At<1>() *= scalar; At<2>() *= scalar;
      At<3>() *= scalar; At<4>() *= scalar; At<5>() *= scalar;
      At<6>() *= scalar; At<7>() *= scalar; At<8>() *= scalar;
      At<9>() *= scalar; At<10>() *= scalar; At<11>() *= scalar;
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