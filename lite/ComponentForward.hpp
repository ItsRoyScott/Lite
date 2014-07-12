#pragma once

namespace lite
{
  class Model;
  class RigidBody;
  class SphereCollision;
  class Transform;

  // Null pointers for easy indexing into GameObject
  //  to retrieve a component by type.
  static const Model*             Model_ = nullptr;
  static const RigidBody*         RigidBody_ = nullptr;
  static const SphereCollision*   SphereCollision_ = nullptr;
  static const Transform*         Transform_ = nullptr;
} // namespace lite