#pragma once

#include "Matrix.hpp"
#include "Vector.hpp"

namespace lite
{
  // Makes working with XMMATRIX less painful.
  class Matrix
  {
  public: // data

    XMMATRIX xm;

  public: // methods

    Matrix()
    {
      xm = XMMatrixIdentity();
    }

    Matrix(const float4x4& f) :
      xm(XMLoadFloat4x4(&f))
    {}

    Matrix(const XMMATRIX& m) :
      xm(m)
    {}

    pair<Matrix, Vector> Inverse() const
    {
      pair<Matrix, Vector> result;
      result.first = XMMatrixInverse(&result.second.xm, xm);
      return result;
    }

    Matrix operator*(const Matrix& b) const
    {
      return XMMatrixMultiply(xm, b.xm);
    }

    operator float4x4() const
    {
      float4x4 f;
      XMStoreFloat4x4(&f, xm);
      return f;
    }
  };
} // namespace lite