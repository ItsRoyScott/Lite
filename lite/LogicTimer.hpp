#pragma once

#include "FrameTimer.hpp"

namespace lite
{
  // Fast timer recommended for game logic because it
  //  takes measurements from the engine's FrameTimer.
  class LogicTimer
  {
  private: // data

    float startTime;

  public: // methods

    // Starts the timer.
    LogicTimer()
    {
      Start();
    }

    // Returns milliseconds since the last call to start.
    float ElapsedMilliseconds()
    {
      return ElapsedSeconds() * 1000.0f;
    }

    // Returns seconds since the last call to start.
    float ElapsedSeconds()
    {
      return FrameTimer::CurrentInstance()->TotalTime() - startTime;
    }

    // Starts or restarts the timer.
    void Start()
    {
      startTime = FrameTimer::CurrentInstance()->TotalTime();
    }
  };
} // namespace lite