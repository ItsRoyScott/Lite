#include "Precompiled.hpp"
#include "Audio.hpp"
#include "FrameTimer.hpp"
#include "Graphics.hpp"
#include "Input.hpp"
#include "Model.hpp"
#include "Reflection.hpp"
#include "Scripting.hpp"
#include "Transform.hpp"
#include "Window.hpp"

int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
  using namespace lite;

  // Initialize our systems.
  auto audio    = Audio();
  auto window   = Window("Lite Game Engine", 960, 540);
  auto graphics = Graphics(window);

  // Set up the scene.
  vector<shared_ptr<ModelInstance>> models;
  static const float limit = 12;
  static const float gap = 3;
  for (float x = -limit; x <= limit; x += gap)
  {
    for (float y = -limit / 2; y <= limit / 2; y += gap)
    {
      for (float z = -limit; z <= limit; z += gap)
      {
        auto model = graphics.AddModel();
        model->Material = "Default";
        model->Mesh = "spongebob.obj";
        XMStoreFloat4x4(&model->Transform, XMMatrixTranslation(x, y, z));
        models.push_back(move(model));
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
    if (Input::IsHeld('A')) graphics.Camera.RotateY(-XM_PI / 60.0f);
    if (Input::IsHeld('D')) graphics.Camera.RotateY(XM_PI / 60.0f);
    if (Input::IsHeld('R')) graphics.Camera.Climb(0.5f);
    if (Input::IsHeld('F')) graphics.Camera.Climb(-0.5f);

    // Update all systems.
    window.Update();
    graphics.Update(frameTimer.DeltaTime());
    audio.Update();

    frameTimer.EndFrame();
  }

  return 0;
}
