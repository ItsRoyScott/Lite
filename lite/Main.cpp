#include "Precompiled.hpp"
#include "Audio.hpp"
#include "FrameTimer.hpp"
#include "GameObject.hpp"
#include "Graphics.hpp"
#include "Input.hpp"
#include "LogicTimer.hpp"
#include "Physics.hpp"
#include "Reflection.hpp"
#include "Scripting.hpp"
#include "Window.hpp"

#include "CollisionComponents.hpp"
#include "Model.hpp"
#include "RigidBody.hpp"
#include "Transform.hpp"

int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
  using namespace lite;

  // Initialize our systems.
  auto audio    = Audio();
  auto physics  = Physics();
  auto window   = Window("Lite Game Engine", 960, 540);
  auto graphics = Graphics(window);

  RegisterComponent<Model>();
  RegisterComponent<PlaneCollision>();
  RegisterComponent<RigidBody>();
  RegisterComponent<SphereCollision>();
  RegisterComponent<Transform>();

  GameObject scene;

  GameObject& floor = scene.AddChild();

  Transform& transform = floor[Transform_];
  transform.LocalPosition.y = -10;
  transform.LocalScale.x = 1000;
  transform.LocalScale.y = 0.01f;
  transform.LocalScale.z = 1000;

  Model& model = floor[Model_];
  model.Color({ 0, .25f, 0, 1.0f });
  model.Material("SolidColor");
  model.Mesh("cube.obj");

  RigidBody& body = floor[RigidBody_];
  body.Mass(0);

  PlaneCollision& collision = floor[PlaneCollision_];
  collision.Direction({0, 1, 0});
  collision.Offset(-10);

  // Set up the scene.
  static const float limit = 3;
  static const float gap = 1;
  for (float x = -limit; x <= limit; x += gap)
  {
    for (float y = -limit / 2; y <= limit / 2; y += gap)
    {
      for (float z = -limit; z <= limit; z += gap)
      {
        GameObject& child = scene.AddChild();

        float shiftX = float(rand() % 5 + 1)/10 - 0.5f;
        float shiftZ = float(rand() % 5 + 1)/10 - 0.5f;

        Transform& transform = child[Transform_];
        transform.LocalPosition = { x+shiftX, y, z+shiftZ };

        Model& model = child[Model_];
        model.Material("Default");
        model.Mesh("spongebob.obj");

        RigidBody& body = child[RigidBody_];
        body.Mass(10);

        SphereCollision& collision = child[SphereCollision_];
        collision.Radius(0.5f);
      }
    }
  }

  auto frameTimer = FrameTimer();

  // Game loop:
  while (window.IsOpen())
  {
    frameTimer.BeginFrame();
    window.SetTitle(string("Lite - ") + to_string((int)frameTimer.FPS()) + " fps");

    if (Input::IsHeld('W')) graphics.Camera.Walk(0.5f);
    if (Input::IsHeld('S')) graphics.Camera.Walk(-0.5f);
    if (Input::IsHeld('Q')) graphics.Camera.Strafe(-0.5f);
    if (Input::IsHeld('E')) graphics.Camera.Strafe(0.5f);
    if (Input::IsHeld('A')) graphics.Camera.RotateY(-XM_PI / 45.0f);
    if (Input::IsHeld('D')) graphics.Camera.RotateY(XM_PI / 45.0f);
    if (Input::IsHeld('R')) graphics.Camera.Climb(0.5f);
    if (Input::IsHeld('F')) graphics.Camera.Climb(-0.5f);

    scene.Update();

    scene.PushToSystems();

    // Update all systems.
    audio.Update();
    physics.Update(frameTimer.DeltaTime());
    window.Update();
    graphics.Update(frameTimer.DeltaTime());

    scene.PullFromSystems();

    frameTimer.EndFrame();
  }

  return 0;
}
