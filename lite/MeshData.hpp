#pragma once

#include "AssimpInclude.hpp"
#include "D3DInfo.hpp"
#include <fstream>
#include "Logging.hpp"

namespace lite
{
  class MeshData
  {
  public: // types

    struct ObjectConstants
    {
      float4x4 world;
      float4 outputColor;
    };

    struct Vertex
    {
      float3 position;
      float3 normal;
      float2 tex;
      float3 tangent;
      float3 bitangent;
    };

  private: // data

    BufferHandle      constantBuffer;
    BufferHandle      indexBuffer;
    vector<uint32_t>  indices;
    string            name;
    BufferHandle      vertexBuffer;
    vector<Vertex>    vertices;

  public: // data

    // AssetImporter import flags.
    static const unsigned ImportSetting = 
      aiProcess_ConvertToLeftHanded |
      aiProcess_PreTransformVertices |
      aiProcessPreset_TargetRealtime_MaxQuality;

    static const DXGI_FORMAT IndexFormat = DXGI_FORMAT_R32_UINT;

  public: // properties

    const BufferHandle& ConstantBuffer() const { return constantBuffer; }

    const BufferHandle& IndexBuffer() const { return indexBuffer; }

    const vector<uint32_t>&  Indices() const { return indices; }

    const string& Name() const { return name; }

    const BufferHandle& VertexBuffer() const { return vertexBuffer; }

    const vector<Vertex>& Vertices() const { return vertices; }

  public: // methods

    MeshData() = default;

    MeshData(const string& name)
    {
      static Assimp::Importer importer;
      D3DInfo& d3d = *D3DInfo::CurrentInstance();

      // Read the mesh data from file using Asset Importer.
      const aiScene* scene = importer.ReadFile(config::Meshes + name, ImportSetting);
      if (!scene || !scene->mNumMeshes)
      {
        Warn("Mesh read failed for " + name);
        return;
      }

      const aiMesh* mesh = scene->mMeshes[0];
      WarnIf(scene->mNumMeshes > 1, "Mesh " + name + " has more sub-meshes than are currently supported");
      
      // Verify mesh texture coordinates and tangents.
      if (!mesh->HasTextureCoords(0))
      {
        Warn("Mesh " + name + " doesn't have texture coordinates");
        return;
      }
      if (!mesh->HasTangentsAndBitangents())
      {
        Warn("Mesh " + name + " doesn't have tangents/bitangents");
        return;
      }

      // Copy all vertices.
      vertices.resize(mesh->mNumVertices);
      for (size_t i = 0; i < mesh->mNumVertices; ++i)
      {
        vertices[i].position  = reinterpret_cast<const float3&>(mesh->mVertices[i]);
        vertices[i].normal    = reinterpret_cast<const float3&>(mesh->mNormals[i]);
        vertices[i].tex = reinterpret_cast<const float2&>(mesh->mTextureCoords[0][i]);
        vertices[i].tangent = reinterpret_cast<const float3&>(mesh->mTangents[i]);
        vertices[i].bitangent = reinterpret_cast<const float3&>(mesh->mBitangents[i]);
      }

      // Copy all indices.
      indices.resize(mesh->mNumFaces * 3);
      for (size_t i = 0; i < mesh->mNumFaces; ++i)
      {
        indices[i * 3 + 0] = mesh->mFaces[i].mIndices[0];
        indices[i * 3 + 1] = mesh->mFaces[i].mIndices[1];
        indices[i * 3 + 2] = mesh->mFaces[i].mIndices[2];
      }

      // Free the loaded scene.
      importer.FreeScene();

      // Create the index buffer.
      D3D11_BUFFER_DESC bufferDesc;
      bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
      bufferDesc.ByteWidth = indices.size() * sizeof(indices[0]);
      bufferDesc.CPUAccessFlags = 0;
      bufferDesc.MiscFlags = 0;
      bufferDesc.StructureByteStride = 0;
      bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
      D3D11_SUBRESOURCE_DATA initData;
      initData.pSysMem = indices.data();
      initData.SysMemPitch = 0;
      initData.SysMemSlicePitch = 0;
      DX(d3d.Device->CreateBuffer(&bufferDesc, &initData, indexBuffer));

      // Create the vertex buffer.
      bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
      bufferDesc.ByteWidth = vertices.size() * sizeof(vertices[0]);
      initData.pSysMem = vertices.data();
      DX(d3d.Device->CreateBuffer(&bufferDesc, &initData, vertexBuffer));

      // Create the constant buffer.
      D3D11_BUFFER_DESC cbDesc;
      ZeroMemory(&cbDesc, sizeof(cbDesc));
      cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
      cbDesc.ByteWidth = sizeof(ObjectConstants);
      cbDesc.Usage = D3D11_USAGE_DEFAULT;
      DX(d3d.Device->CreateBuffer(&cbDesc, nullptr, constantBuffer));
    }

    virtual ~MeshData() {}

    // Whether the mesh loaded successfully.
    bool IsLoaded() const
    {
      return bool(vertexBuffer) && bool(indexBuffer);
    }
  };
} // namespace lite