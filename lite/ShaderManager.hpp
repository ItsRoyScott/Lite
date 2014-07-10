#pragma once

#include "ComHandle.hpp"
#include "Essentials.hpp"
#include "ShaderData.hpp"
#include <unordered_map>

namespace lite
{
  // Stores maps of all shaders.
  class ShaderManager : public Singleton<ShaderManager>
  {
  private: // data

    // Map of all shaders for each type.
    unordered_map<string, ShaderData> shaders[ShaderTypeCount];

  public: // methods

    ShaderData& Get(ShaderType type, const string& name)
    {
      // Create the shader if it doesn't exist.
      auto it = shaders[type].find(name);
      if (it == shaders[type].end())
      {
        it = shaders[type].emplace(name, name).first;
      }
      return it->second;
    }
  };
} // namespace lite