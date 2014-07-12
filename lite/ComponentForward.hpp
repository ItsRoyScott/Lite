#pragma once

namespace lite
{
  class Model;
  class Transform;

  // Null pointers for easy indexing into GameObject
  //  to retrieve a component by type.
  static const Model*         Model_ = nullptr;
  static const Transform*     Transform_ = nullptr;
} // namespace lite