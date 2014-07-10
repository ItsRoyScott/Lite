#pragma once

#include "Component.hpp"
#include "Graphics.hpp"
#include "ModelInstance.hpp"

namespace lite
{
  class Model : public Component<Model>
  {
  private: // data

    shared_ptr<ModelInstance> model;

  public: // properties

    // Name of the material used to render the mesh.
    const string& Material() const { return model->Material; }
    void Material(string material) { model->Material = move(material); }

    // Name of the mesh including extension.
    const string& Mesh() const { return model->Mesh; }
    void Mesh(string mesh) { model->Mesh = move(mesh); }

  public: // methods

    Model()
    {
      model = Graphics::CurrentInstance()->AddModel();
    }
  };
} // namespace lite