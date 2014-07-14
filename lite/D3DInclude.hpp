#pragma once

#include "Essentials.hpp"
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

namespace lite 
{
  typedef XMFLOAT2 float2;
  typedef XMFLOAT3 float3;
  typedef XMFLOAT4 float4;

  inline ostream& operator<<(ostream& os, const float4& f)
  {
    return os << f.x << "," << f.y << "," << f.z << "," << f.w;
  }
} // namespace lite

// DirectX call macro which automatically checks the HRESULT for failure.
#define DX(x) DO_IF(FAILED(x), throw runtime_error("Direct3D call failed: " #x))
