#pragma once

#include "KeyboardBuffer.hpp"
#include "MouseBuffer.hpp"

namespace lite
{
  // Manages input buffers used to query per-frame states of devices connected to the PC.
  //  Virtual-key codes: http://msdn.microsoft.com/en-us/library/windows/desktop/dd375731%28v=vs.85%29.aspx
  class Input : public Singleton<Input>
  {
  private: // data

    KeyboardBuffer keyboard;
    MouseBuffer mouse;

  public: // data

    // Stores per-frame keyboard info.
    const KeyboardBuffer& Keyboard() const { return keyboard; }

    // Stores per-frame mouse info.
    const MouseBuffer& Mouse() const { return mouse; }

  public: // methods

    // Change of position in X since last frame.
    static int GetMouseDeltaX()
    {
      return Instance().Mouse().GetDeltaX();
    }

    // Change of position in Y since last frame.
    static int GetMouseDeltaY()
    {
      return Instance().Mouse().GetDeltaY();
    }

    // The wheel movements in a direction since last frame.
    //  Positive: rotated up, Negative: rotated down
    static int GetMouseWheelDelta()
    {
      return Instance().Mouse().GetWheelDelta();
    }

    // The current X position.
    static int GetMouseX()
    {
      return Instance().Mouse().GetX();
    }

    // The current Y position.
    static int GetMouseY()
    {
      return Instance().Mouse().GetY();
    }

    // The button is down this frame and last frame.
    static bool IsHeld(MouseButton button)
    {
      return Instance().Mouse().IsHeld(button);
    }

    // The button is down this frame and last frame.
    static bool IsHeld(uint8_t vkKeyCode)
    {
      return Instance().Keyboard().IsHeld(vkKeyCode);
    }

    // The button is up this frame but down last frame.
    static bool IsReleased(MouseButton button)
    {
      return Instance().Mouse().IsReleased(button);
    }

    // The button is up this frame but down last frame.
    static bool IsReleased(uint8_t vkKeyCode)
    {
      return Instance().Keyboard().IsReleased(vkKeyCode);
    }

    // The button is down this frame but up last frame.
    static bool IsTriggered(MouseButton button)
    {
      return Instance().Mouse().IsTriggered(button);
    }

    // The button is down this frame but up last frame.
    static bool IsTriggered(uint8_t vkKeyCode)
    {
      return Instance().Keyboard().IsTriggered(vkKeyCode);
    }

    // The button is up this frame and last frame.
    static bool IsUp(MouseButton button)
    {
      return Instance().Mouse().IsUp(button);
    }

    // The button is up this frame and last frame.
    static bool IsUp(uint8_t vkKeyCode)
    {
      return Instance().Keyboard().IsUp(vkKeyCode);
    }

  private: // methods

    Input() = default;

    friend Singleton < Input > ;
  };
} // namespace lite