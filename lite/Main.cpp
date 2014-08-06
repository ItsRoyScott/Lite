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
  auto physics = Physics(true, {0, -6, 0});
  auto window   = Window("Lite Game Engine", 960, 540);
  auto graphics = Graphics(window);

  RegisterComponent<Model>();
  RegisterComponent<PlaneCollision>();
  RegisterComponent<RigidBody>();
  RegisterComponent<SphereCollision>();
  RegisterComponent<Transform>();

  Note(Reflection::Instance());

  auto scene = GameObject("Scene.txt");

  auto& spongebobPrefab = *GetPrefab("Bee");
  auto frameTimer = FrameTimer();

  // Game loop:
  while (window.IsOpen())
  {
    frameTimer.BeginFrame();
    window.Title(string("Lite - ") + to_string((int) frameTimer.FPS()) + " fps");
    
    Scripting::Instance().DoString(1 + R"_LuaScript_(
        --local Camera = lite.Graphics.CurrentInstance.Camera
        local Input = lite.Input
        local walkAmount = 0.5
        
        --for key,value in pairs(lite.Graphics.CurrentInstance.Camera) do print(key,value) end
        if ( Input.IsKeyHeld(string.byte("W")) ) then lite.Graphics.CurrentInstance:GetCamera():Walk(walkAmount)     end
        if ( Input.IsKeyHeld(string.byte("S")) ) then lite.Graphics.CurrentInstance:GetCamera():Walk(-walkAmount)    end
        if ( Input.IsKeyHeld(string.byte("A")) ) then lite.Graphics.CurrentInstance:GetCamera():Strafe(-walkAmount)  end
        if ( Input.IsKeyHeld(string.byte("D")) ) then lite.Graphics.CurrentInstance:GetCamera():Strafe(walkAmount)   end
        if ( Input.IsKeyHeld(string.byte("Q")) ) then lite.Graphics.CurrentInstance:GetCamera():Climb(walkAmount)    end
        if ( Input.IsKeyHeld(string.byte("E")) ) then lite.Graphics.CurrentInstance:GetCamera():Climb(-walkAmount)   end
      )_LuaScript_");
    
    if (Input::IsTriggered(VK_ESCAPE))  window.Destroy();
    if (Input::IsTriggered(VK_F1))      DebugDrawCollisions() = !DebugDrawCollisions();

    if (Input::IsTriggered(VK_SPACE))
    {
      GameObject& child = scene.AddChild(spongebobPrefab);
      child[Transform_].LocalPosition = graphics.Camera.Position();
      child[RigidBody_].AddForce(Vector(graphics.Camera.Look()) * 300);
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