#pragma once

#include "Essentials.hpp"
#include <fstream>
#include "ShaderManager.hpp"

namespace lite
{
  // Pairs shaders with a texture to define a type of 
  //  renderable "material" in the game world.
  class MaterialDescription
  {
  private: // data

    string name = string();
    string pixelShader = string();
    string texture = string();
    string vertexShader = string();

  public: // properties

    // Name of the material.
    const string& Name() const { return name; }

    // Name of the pixel shader.
    const string& PixelShader() const { return pixelShader; }

    // Texture applied to the object.
    const string& Texture() const { return texture; }

    // Name of the vertex shader.
    const string& VertexShader() const { return vertexShader; }

  public: // methods

    // Reads the material from the given path.
    MaterialDescription(string name_) :
      name(move(name_))
    {
      // Open and read the file.
      auto file = ifstream(config::Materials + name + ".txt");
      if (!file.is_open())
      {
        Warn("Material file " + config::Materials + name + ".txt" + " could not be opened");
        return;
      }

      // Read all properties.
      while (file.good())
      {
        string key, value;
        file >> key >> value;
        if (key.empty() || value.empty()) break;
        SetProperty(move(key), move(value));
      }
    }

    // Returns whether the material properly loaded.
    bool IsLoaded() const
    {
      return pixelShader.size() != 0 && texture.size() != 0 && vertexShader.size() != 0;
    }

  private: // methods

    void SetProperty(const string& key, string value)
    {
      if (key == "vs") vertexShader = move(value);
      else if (key == "ps") pixelShader = move(value);
      else if (key == "tx") texture = move(value);
    }
  };
} // namespace lite