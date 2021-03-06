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

    // Whether backfaces are culled.
    const bool& BackfaceCulling() const { return model->BackfaceCulling; }
    void BackfaceCulling(bool b) { model->BackfaceCulling = b; }

    // Color of the model (ignored unless the shader uses it).
    const float4& Color() const { return model->Color; }
    void Color(const float4& c) { model->Color = c; }

    // Name of the material used to render the mesh.
    const string& Material() const { return model->Material; }
    void Material(string material) { model->Material = move(material); }

    // Name of the mesh including extension.
    const string& Mesh() const { return model->Mesh; }
    void Mesh(string mesh) { model->Mesh = move(mesh); }

    // Texture name overriding the material's default texture.
    const string& Texture() const { return model->Texture; }
    void Texture(string texture) { model->Texture = move(texture); }

  public: // methods

    Model()
    {
      model = Graphics::CurrentInstance()->AddModel();
    }

    Model(const Model& b)
    {
      model = Graphics::CurrentInstance()->AddModel();

      BackfaceCulling(b.BackfaceCulling());
      Color(b.Color());
      Material(b.Material());
      Mesh(b.Mesh());
      Texture(b.Texture());
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

  template<>
  struct Binding<Model> : BindingBase<Model>
  {
    Binding()
    {
      Bind(
        Constructor<>,
        "BackfaceCulling", Const(&T::BackfaceCulling), NonConst(&T::BackfaceCulling),
        "Color", Const(&T::Color), NonConst(&T::Color),
        "Material", Const(&T::Material), NonConst(&T::Material),
        "Mesh", Const(&T::Mesh), NonConst(&T::Mesh),
        "Texture", Const(&T::Texture), NonConst(&T::Texture));
    }
  };
} // namespace lite