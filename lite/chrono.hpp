#pragma once

#include <chrono>
#include "Essentials.hpp"
#include "WindowsInclude.h"

namespace lite
{
  using std::chrono::duration;
  using std::chrono::duration_cast;
  using std::ratio;
  using std::chrono::time_point;

  // Replacement for VC++ incorrect implementation.
  class high_resolution_clock
  {
  public: // types

    typedef double                            rep;
    typedef ratio<1, 1>                       period;
    typedef duration<rep, period>             duration;
    typedef time_point<high_resolution_clock> time_point;

  public: // properties

    static const bool is_monotonic = true;
    static const bool is_steady = true;

  public: // methods

    static time_point now()
    {
      // Get the current performance counter ticks.
      int64_t counter = 0;
      QueryPerformanceCounter((LARGE_INTEGER*)&counter);

      // Subtract the counter from the value at program start
      //  so a double can hold it.
      int64_t idt = counter - start_count();
      double dt = static_cast<double>(idt);

      WarnIf(static_cast<int64_t>(dt) != idt, "'double' cannot hold QueryPerformanceCounter delta " << idt);

      return time_point(duration(dt / frequency()));
    }

  private: // methods

    // Number of ticks per second.
    static int64_t frequency()
    {
      static int64_t freq = -1;
      if (freq == -1)
      {
        QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
      }
      return freq;
    }

    // Counter value at the start of the application.
    static int64_t start_count()
    {
      static int64_t count = 0;
      if (count == 0)
      {
        QueryPerformanceCounter((LARGE_INTEGER*)&count);
      }
      return count;
    }
      
    // Number of ticks per second.
    static double tick_duration()
    {
      double dur = 1.0 / double(frequency());
      return dur;
    }
  };

  // Stopwatch-like object which keeps track of elapsed time.
  class high_resolution_timer
  {
  private: // data

    // Time recorded when start() was last called.
    high_resolution_clock::time_point startTime;

  public: // methods

    // Constructs timer object; calls start().
    explicit high_resolution_timer()
    {
      start();
    }

    // The current elapsed time as a duration object.
    high_resolution_clock::duration elapsed_duration() const
    {
      high_resolution_clock::time_point endTime = high_resolution_clock::now();
      return endTime - startTime;
    }

    // Microseconds that have elapsed since start() was called.
    double elapsed_microseconds() const
    {
      return duration_cast<duration<double, micro>>(elapsed_duration()).count();
    }

    // Milliseconds that have elapsed since start() was called.
    double elapsed_milliseconds() const
    {
      return duration_cast<duration<double, milli>>(elapsed_duration()).count();
    }

    // Seconds that have elapsed since start() was called.
    double elapsed_seconds() const
    {
      return elapsed_duration().count();
    }

    // Starts or restarts the timer.
    void start()
    {
      startTime = high_resolution_clock::now();
    }
  };
} // namespace lite