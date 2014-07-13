#pragma once

#include "D3DInfo.hpp"
#include "Logging.hpp"
#include "WICTextureLoader.h"

namespace lite
{
  class TextureData
  {
  private: // data

    string name;
    ShaderResourceViewHandle resourceView;
    ResourceHandle texture;

  public: // properties

    const string& Name = name;
    const ShaderResourceViewHandle& ResourceView = resourceView;
    const ResourceHandle& Texture = texture;

  public: // methods

    TextureData(string name_) :
      name(move(name_))
    {
      if (name.empty())
      {
        name = config::DefaultTexture;
      }

      // Use the WIC texture loader.
      HRESULT hr = DirectX::CreateWICTextureFromFile(
        D3DInfo::CurrentInstance()->Device,
        MultibyteToWideChar(config::Textures + name).c_str(),
        Texture,
        ResourceView);

      WarnIf(FAILED(hr), "Load failed for texture " + name);
    }

    // Whether the texture successfully loaded.
    bool IsLoaded() const
    {
      return bool(ResourceView) && bool(Texture);
    }
  };
} // namespace lite