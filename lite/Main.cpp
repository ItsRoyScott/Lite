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

static void SetupScene();
static void UpdateScene();

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
  transform.LocalPosition.y = -2;
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
  collision.Offset(-2);

  GameObject spongebobPrefab;
  {
    spongebobPrefab[Transform_];

    Model& model = spongebobPrefab[Model_];
    model.Material("Default");
    model.Mesh("spongebob.obj");

    RigidBody& body = spongebobPrefab[RigidBody_];
    body.Mass(1);

    SphereCollision& collision = spongebobPrefab[SphereCollision_];
    collision.Radius(0.5f);
  }

  auto frameTimer = FrameTimer();

  // Game loop:
  while (window.IsOpen())
  {
    frameTimer.BeginFrame();
    window.Title(string("Lite - ") + to_string((int)frameTimer.FPS()) + " fps");

    if (Input::IsHeld('W')) graphics.Camera.Walk(0.5f);
    if (Input::IsHeld('S')) graphics.Camera.Walk(-0.5f);
    if (Input::IsHeld('A')) graphics.Camera.Strafe(-0.5f);
    if (Input::IsHeld('D')) graphics.Camera.Strafe(0.5f);
    if (Input::IsHeld('Q')) graphics.Camera.Climb(0.5f);
    if (Input::IsHeld('E')) graphics.Camera.Climb(-0.5f);

    if (Input::IsTriggered(VK_SPACE))
    {
      GameObject& child = scene.AddChild(spongebobPrefab);
      child[Transform_].LocalPosition = graphics.Camera.Position();
      child[RigidBody_].AddForce(Vector(graphics.Camera.Look()) * 1000);
    }

    graphics.Camera.RotateY((float) Input::GetMouseDeltaX() / 100);
    graphics.Camera.Pitch((float) Input::GetMouseDeltaY() / 100);

    scene.Update();

    scene.PushToSystems();

    // Update all systems.
    audio.Update();
    physics.Update(frameTimer.IdealDeltaTime());
    window.Update();
    graphics.Update(frameTimer.DeltaTime());

    scene.PullFromSystems();

    frameTimer.EndFrame();
  }

  return 0;
}