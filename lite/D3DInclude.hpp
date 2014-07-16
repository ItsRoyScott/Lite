#pragma once

#include "Essentials.hpp"
#include "Reflection.hpp"
#include "WindowsInclude.h"

#pragma warning(push)
#pragma warning(disable: 4005)
#include "D3D/d3dcompiler.h"
#include "D3D/d3d11shader.h"
#include "D3D/d3d11.h"
#include "D3D/xnamath.h"
#pragma warning(pop)

#pragma comment(lib, "D3D/D3D11.lib")
#pragma comment(lib, "D3D/D3DCompiler.lib")

#ifdef _DEBUG
#pragma comment(lib, "D3D/WICTextureLoader-d.lib")
#else
#pragma comment(lib, "D3D/WICTextureLoader.lib")
#endif

namespace lite 
{
  typedef XMFLOAT2 float2;
  typedef XMFLOAT3 float3;
  typedef XMFLOAT4 float4;

  inline ostream& operator<<(ostream& os, const float3& f)
  {
    return os << f.x << " " << f.y << " " << f.z;
  }
  inline istream& operator>>(istream& is, float3& f)
  {
    return is >> f.x >> f.y >> f.z;
  }

  inline ostream& operator<<(ostream& os, const float4& f)
  {
    return os << f.x << " " << f.y << " " << f.z << " " << f.w;
  }
  inline istream& operator>>(istream& is, float4& f)
  {
    return is >> f.x >> f.y >> f.z >> f.w;
  }

  reflect(float3, "float3", Constructor<float3>);
  reflect(float4, "float4", Constructor<float4>);
} // namespace lite

// DirectX call macro which automatically checks the HRESULT for failure.
#define DX(x) DO_IF(FAILED(x), throw runtime_error("Direct3D call failed: " #x))
