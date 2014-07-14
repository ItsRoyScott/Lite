#pragma once

#include "FrameTimer.hpp"
#include "Reflection.hpp"

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
    float ElapsedMilliseconds() const
    {
      return ElapsedSeconds() * 1000.0f;
    }

    // Returns seconds since the last call to start.
    float ElapsedSeconds() const
    {
      return FrameTimer::CurrentInstance()->TotalTime() - startTime;
    }

    // Starts or restarts the timer.
    void Start()
    {
      startTime = FrameTimer::CurrentInstance()->TotalTime();
    }
  };

  reflect(LogicTimer,
    "ElapsedMilliseconds", &T::ElapsedMilliseconds,
    "ElapsedSeconds", &T::ElapsedSeconds,
    "Start", &T::Start);
} // namespace lite