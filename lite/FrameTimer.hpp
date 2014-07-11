#pragma once

#include "chrono.hpp"
#include "Input.hpp"

namespace lite
{
  // Specialized timer for measuring frame length and 
  //  delta time between consecutive frames.
  class FrameTimer : public LightSingleton<FrameTimer>
  {
  private: // data

    float actualDeltaTime = 0;
    float deltaTime = 0;
    float frameLength = 0;
    float idealFramerate = 60;
    bool  lockFramerate = true;
    bool  stepMode = false;
    high_resolution_timer timer;
    float totalTime = 0;

  public: // properties

    // Actual time between the BeginFrame() calls for this frame and the last.
    const float& ActualDeltaTime() const { return actualDeltaTime; }

    // Time between frames used for updating systems and game logic.
    const float& DeltaTime() const { return deltaTime; }

    // Returns the current FPS.
    float FPS() const { return 1.0f / frameLength; }

    // Time between BeginFrame() and EndFrame() calls for a frame.
    const float& FrameLength() const { return frameLength; }

    // What the ideal fps is. (used for locking the framerate)
    const float& IdealFramerate() const { return idealFramerate; }
    void IdealFramerate(float value) { idealFramerate = value; }

    // Whether the framerate should be locked on EndFrame.
    const bool& LockFramerate() const { return lockFramerate; }
    void LockFramerate(bool value) { lockFramerate = value; }

    const float& TotalTime() const { return totalTime; }

  public: // methods

    // Called at the start of a frame; saves frame dt.
    //  Any calls to lock framerate should be made after 
    //  EndFrame() and before BeginFrame().
    void BeginFrame()
    {
      deltaTime = static_cast<float>(timer.elapsed_seconds());

      // If we have an extraordinarily slow dt, the user is likely debugging.
      static const float debuggerDeltaTime = 1.0f;
      if (deltaTime > debuggerDeltaTime)
      {
        // Debugger has been detected. Use the ideal dt.
        deltaTime = 1.0f / idealFramerate;
      }

      totalTime += deltaTime;

      timer.start();
    }

    // Called at the end of a frame; saves frame length.
    //  Any calls to lock framerate should be made after 
    //  EndFrame() and before BeginFrame().
    void EndFrame()
    {
      frameLength = static_cast<float>(timer.elapsed_seconds());
      if (lockFramerate)
      {
        Lock();
      }
    }

  private: // methods

    // Locks the framerate.
    void Lock()
    {
      float frameTime = frameLength;
      float idealFrameTime = 1.0f / idealFramerate;

      // Sleep while this frame has plenty of time left.
      static const float twoMilliseconds = 0.002f;
      while (idealFrameTime - frameTime > twoMilliseconds)
      {
        Sleep(1);
        frameTime = static_cast<float>(timer.elapsed_seconds());
      }

      // Wait for the ideal frame time to lapse.
      while (frameTime < idealFrameTime)
      {
        frameTime = static_cast<float>(timer.elapsed_seconds());
      }
    }
  };
} // namespace lite