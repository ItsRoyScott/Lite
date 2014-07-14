#pragma once

#include "Component.hpp"
#include "GameObject.hpp"
#include "Graphics.hpp"
#include "ModelInstance.hpp"
#include "Transform.hpp"

namespace lite
{
  class Model : public Component<Model>
  {
  private: // data

    shared_ptr<ModelInstance> model;

  public: // properties

    // Color of the model (ignored unless the shader uses it).
    const float4& Color() const { return model->Color; }
    void Color(const float4& c) { model->Color = c; }

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

    Model(const Model& b)
    {
      model = Graphics::CurrentInstance()->AddModel();
    
      Color(b.Color());
      Material(b.Material());
      Mesh(b.Mesh());
    }

  private: // methods

    void Activate() override
    {
      model->IsVisible = true;
    }

    void Deactivate() override
    {
      model->IsVisible = false;
    }

    void PushToSystems() override
    {
      Transform& tfm = OwnerReference()[Transform_];
      model->Transform = tfm.GetWorldMatrix();
    }
  };

  reflect(Model,
    "Color", Getter(&T::Color), Setter(&T::Color),
    "Material", Getter(&T::Material), Setter(&T::Material),
    "Mesh", Getter(&T::Mesh), Setter(&T::Mesh));
} // namespace lite