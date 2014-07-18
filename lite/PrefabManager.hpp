#pragma once

#include "Essentials.hpp"
#include "GameObject.hpp"
#include "PathInfo.hpp"
#include <unordered_map>

namespace lite
{
  class PrefabManager : public Singleton<PrefabManager>
  {
  private: // data

    unordered_map<string, GameObject> prefabs;

  public: // methods

    PrefabManager()
    {
      // For each prefab file in the Prefabs directory:
      auto files = PathInfo(config::Prefabs).Files();
      for (const string& prefabFile : files)
      {
        // Create a new inactive game object representing the new prefab,
        //  and use the base name of the file as the prefab name.
        auto prefabFileInfo = PathInfo(prefabFile);
        pair<string, GameObject> newPair = { prefabFileInfo.BaseFilename(), GameObject(false) };

        // Load the prefab into the game object.
        newPair.second.LoadFromFile(prefabFileInfo.Filename(), config::Prefabs);

        // Insert the pair into the hash-map.
        prefabs.insert(move(newPair));
      }
    }

    const GameObject* GetPrefab(const string& name)
    {
      auto it = prefabs.find(name);
      if (it == prefabs.end()) return nullptr;
      return &it->second;
    }
  };

  inline const GameObject* GetPrefab(const string& name)
  {
    return PrefabManager::Instance().GetPrefab(name);
  }
} // namespace lite