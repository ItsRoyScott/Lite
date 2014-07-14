#pragma once

#include <bitset>
#include "Essentials.hpp"
#include "Window.hpp"
#include "WindowsInclude.h"

namespace lite
{
  enum class MouseButton
  {
    Left, Right, Middle
  };
  static const size_t MouseButtonCount = 3;

  class MouseBuffer
  {
  private: // data

    // Buffers for mouse button input.
    bitset<MouseButtonCount> buffers[2];
    bitset<MouseButtonCount>* currentBuffer = nullptr;
    bitset<MouseButtonCount>* previousBuffer = nullptr;

    // Position data for current and previous frame.
    int currentX = 0, currentY = 0, previousX = 0, previousY = 0;

    // Number of wheel turns since last frame.
    int wheelDelta = 0;

    EventHandler onWindowMessage = { "WindowMessage", this, &MouseBuffer::OnWindowMessage };
    EventHandler onWindowUpdate = { "WindowUpdate", this, &MouseBuffer::OnWindowUpdate };

  public: // methods

    MouseBuffer()
    {
      currentBuffer = &buffers[0];
      previousBuffer = &buffers[1];
    }

    // Change of position in X since last frame.
    int GetDeltaX() const
    {
      return currentX - previousX;
    }

    // Change of position in Y since last frame.
    int GetDeltaY() const
    {
      return currentY - previousY;
    }

    // The wheel movements in a direction since last frame.
    //  Positive: rotated up, Negative: rotated down
    int GetWheelDelta() const
    {
      return wheelDelta;
    }

    // The current X position.
    int GetX() const
    {
      return currentX;
    }

    // The current Y position.
    int GetY() const
    {
      return currentY;
    }

    // The button is down this frame and last frame.
    bool IsHeld(MouseButton button) const
    {
      return Current(button) && Previous(button);
    }

    // The button is up this frame but down last frame.
    bool IsReleased(MouseButton button) const
    {
      return !Current(button) && Previous(button);
    }

    // The button is down this frame but up last frame.
    bool IsTriggered(MouseButton button) const
    {
      return Current(button) && !Previous(button);
    }

    // The button is up this frame and last frame.
    bool IsUp(MouseButton button) const
    {
      return !Current(button) && !Previous(button);
    }

  private: // methods

    // Returns a reference to the current state of a button.
    bitset<MouseButtonCount>::reference Current(MouseButton button) const
    {
      return (*currentBuffer)[(unsigned) button];
    }

    void HandleMouseMove(HWND hwnd, LPARAM lparam)
    {
      // Convert screen coordinates to client-area coordinates.
      POINT point;
      point.x = (short) LOWORD(lparam);
      point.y = (short) HIWORD(lparam);
      ScreenToClient(hwnd, &point);

      currentX = point.x;
      currentY = point.y;

      // Ignore mouse delta if the cursor teleported.
      if (abs(currentX - previousX) > 200 || abs(currentY - previousY) > 200)
      {
        previousX = currentX;
        previousY = currentY;
      }
    }

    // Processes mouse input events.
    void OnWindowMessage(EventData& eventData)
    {
      switch (eventData.Get<UINT>("uMsg"))
      {
      case WM_LBUTTONDOWN:
        Current(MouseButton::Left) = true;
        break;
      case WM_LBUTTONUP:
        Current(MouseButton::Left) = false;
        break;
      case WM_MBUTTONDOWN:
        Current(MouseButton::Middle) = true;
        break;
      case WM_MBUTTONUP:
        Current(MouseButton::Middle) = false;
        break;
      case WM_MOUSEMOVE:
        HandleMouseMove(eventData.Get<HWND>("hWnd"), eventData.Get<LPARAM>("lParam"));
        break;
      case WM_MOUSEWHEEL:
        wheelDelta = GET_WHEEL_DELTA_WPARAM(eventData.Get<WPARAM>("wParam"));
        break;
      case WM_RBUTTONDOWN:
        Current(MouseButton::Right) = true;
        break;
      case WM_RBUTTONUP:
        Current(MouseButton::Right) = false;
        break;
      default:
        return;
      }

      eventData["handled"] = true;
    }

    // Sets up buffers for the new data.
    void OnWindowUpdate(EventData&)
    {
      // Copy last frame's data into the previous buffer.
      *previousBuffer = *currentBuffer;

      previousX = currentX;
      previousY = currentY;

      wheelDelta = 0;
    }

    // Returns a reference to the previous state of a button.
    bitset<MouseButtonCount>::reference Previous(MouseButton button) const
    {
      return (*previousBuffer)[(unsigned) button];
    }
  };
} // namespace lite