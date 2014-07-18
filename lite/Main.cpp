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

#include "LuaCppInterfaceInclude.hpp"
#include "PrefabManager.hpp"

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

  scene.LoadFromFile("Scene.txt");

  auto& spongebobPrefab = *GetPrefab("Bee");

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
    
    if (Input::IsTriggered(VK_ESCAPE)) window.Destroy();

    if (Input::IsTriggered(VK_SPACE))
    {
      GameObject& child = scene.AddChild(spongebobPrefab);
      child[Transform_].LocalPosition = graphics.Camera.Position();
      child[RigidBody_].AddForce(Vector(graphics.Camera.Look()) * 400);
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