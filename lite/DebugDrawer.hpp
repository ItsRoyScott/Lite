#pragma once

#include "ComHandle.hpp"
#include "D3DInclude.hpp"
#include "Essentials.hpp"
#include "Matrix.hpp"
#include "ModelInstance.hpp"

namespace lite
{
  enum class DebugColor
  {
    Black,
    Blue,
    Green,
    Orange,
    Purple,
    Red,
    White,
    Yellow,
    Count
  };

  class DebugSphere : public ModelInstance
  {
  public: // methods

    DebugSphere(float3 position, float3 scale, float3 color)
    {
      Color = { color.x, color.y, color.z, 1 };
      Material = "SolidColor";
      Mesh = "sphere.obj";
      Topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;

      XMMATRIX transMatrix = XMMatrixTranslation(position.x, position.y, position.z);
      XMMATRIX scaleMatrix = XMMatrixScaling(scale.x, scale.y, scale.z);
      XMStoreFloat4x4(&Transform, XMMatrixMultiply(scaleMatrix, transMatrix));
    }
  };

  class DebugDrawer : public Singleton<DebugDrawer>
  {
  private: // data

    vector<DebugSphere> spheres;

  public: // methods

    DebugDrawer()
    {}

    void DrawSphere(float3 position, float3 scale, float3 color)
    {
      spheres.emplace_back(position, scale, color);
    }

    void Update()
    {
      // Draw all spheres.
      for (auto& sphere : spheres)
      {
        sphere.Draw();
      }

      // Clear all data to prepare for the next frame.
      spheres.clear();
    }
  };

  inline void DrawSphere(
    float3 position, 
    float3 scale = { 1, 1, 1 }, 
    float3 color = { 1, 1, 1 })
  {
    DebugDrawer::Instance().DrawSphere(position, scale, color);
  }
} // namespace lite