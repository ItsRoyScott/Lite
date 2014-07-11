#pragma once

#include "WindowsInclude.h"

namespace lite
{
  // Fast timer recommended for game logic because timeGetTime
  //  performs significantly better than QueryPerformanceCounter.
  class LogicTimer
  {
  private: // data

    DWORD startTime;

  public: // methods

    // Starts the timer.
    LogicTimer(bool start = true)
    {
      if (start)
      {
        Start();
      }
    }

    // Returns milliseconds since the last call to start.
    float ElapsedMilliseconds()
    {
      return float(timeGetTime() - startTime);
    }

    // Returns seconds since the last call to start.
    float ElapsedSeconds()
    {
      return ElapsedMilliseconds() / 1000.0f;
    }

    // Starts or restarts the timer.
    void Start()
    {
      startTime = timeGetTime();
    }
  };
} // namespace lite