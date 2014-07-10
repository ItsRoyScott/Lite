#pragma once

#include "GraphicsResourceManager.hpp"
#include "ShaderManager.hpp"

namespace lite
{
  class ModelInstance
  {
  public: // data

    // Name of the material used to draw the model.
    string Material;

    // Name of the mesh.
    string Mesh;

    // World transform matrix.
    XMFLOAT4X4 Transform;

  public: // methods

    void Draw()
    {
      // Get the material and mesh data.
      MaterialDescription& material = MaterialManager::Instance()[Material];
      MeshData& mesh = MeshManager::Instance()[Mesh];
      if (!material.IsLoaded() || !mesh.IsLoaded()) return;

      // Gather the resources specified by the material description.
      ShaderData& vs = ShaderManager::Instance().Get(VertexShader, material.VertexShader());
      ShaderData& ps = ShaderManager::Instance().Get(PixelShader, material.PixelShader());
      TextureData& texture = TextureManager::Instance()[material.Texture()];
      if (!vs.IsLoaded() || !ps.IsLoaded() || !texture.IsLoaded()) return;

      D3DInfo& d3d = *D3DInfo::CurrentInstance();
      UINT vertexStride = sizeof(MeshData::Vertex);
      UINT vertexOffset = 0;

      // Draw.
      MeshData::ObjectConstants constants;
      XMStoreFloat4x4(&constants.world, XMMatrixTranspose(XMLoadFloat4x4(&Transform)));
      d3d.Context->UpdateSubresource(mesh.ConstantBuffer(), 0, nullptr, &constants, 0, 0);
      d3d.Context->IASetVertexBuffers(0, 1, mesh.VertexBuffer(), &vertexStride, &vertexOffset);
      d3d.Context->IASetIndexBuffer(mesh.IndexBuffer(), mesh.IndexFormat, 0);
      d3d.Context->VSSetConstantBuffers(1, 1, mesh.ConstantBuffer());
      d3d.Context->VSSetShader(vs.GetVertexShader(), nullptr, 0);
      d3d.Context->PSSetConstantBuffers(1, 1, mesh.ConstantBuffer());
      d3d.Context->PSSetShader(ps.GetPixelShader(), nullptr, 0);
      d3d.Context->PSSetShaderResources(0, 1, texture.ResourceView);
      d3d.Context->PSSetSamplers(0, 1, d3d.LinearSampler);
      d3d.Context->DrawIndexed(mesh.Indices().size(), 0, 0);
    }
  };
} // namespace lite