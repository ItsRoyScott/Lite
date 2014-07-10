#pragma once

#include "MaterialDescription.hpp"
#include "MeshData.hpp"
#include "ShaderData.hpp"
#include "TextureData.hpp"

namespace lite
{
  // Manages a graphical resource given the type of the resource.
  template <class T>
  class GraphicsResourceManager : public Singleton<GraphicsResourceManager<T>>
  {
  protected: // data

    // Hash-map of the resources.
    unordered_map<string, T> objects;

  public: // methods

    // Returns a resource by name. Creates the resource
    //  if it doesn't yet exist.
    T& operator[](const string& name)
    {
      // Create the object if it doesn't exist.
      auto it = objects.find(name);
      if (it == objects.end())
      {
        it = objects.emplace(name, name).first;
      }
      return it->second;
    }
  };

  typedef GraphicsResourceManager<MeshData>    MeshManager;
  typedef GraphicsResourceManager<TextureData> TextureManager;

  // Graphics manager for materials which loads all on startup.
  class MaterialManager : public GraphicsResourceManager < MaterialDescription >
  {
  public: // methods

    // Reads all materials from the materials assets folder.
    MaterialManager()
    {
      // Gather info about the path to the materials folder.
      auto pathInfo = PathInfo(config::Materials);
      if (!pathInfo.Valid() || pathInfo.Files().empty()) return;

      // Iterate over files in the materials folder.
      for (const string& file : pathInfo.Files())
      {
        // Add the material to the map if it loads.
        MaterialDescription material(file);
        if (material.IsLoaded())
        {
          objects.emplace(material.Name(), move(material));
        }
      }
    }
  };
} // namespace lite