#pragma once

#include <bitset>
#include "EventHandler.hpp"
#include "WindowsInclude.h"

namespace lite
{
  // Stores per-frame states of the keyboard.
  //  Virtual-key codes: http://msdn.microsoft.com/en-us/library/windows/desktop/dd375731%28v=vs.85%29.aspx
  class KeyboardBuffer : LightSingleton<KeyboardBuffer>
  {
  private: // data

    // Maximum number of keys supported.
    static const size_t maxKeys = 256;

    // Buffers to store current-frame and previous-frame states.
    bitset<maxKeys> buffers[2];
    bitset<maxKeys>* currentBuffer = nullptr;
    bitset<maxKeys>* previousBuffer = nullptr;

    EventHandler onWindowMessage = { "WindowMessage", this, &KeyboardBuffer::OnWindowMessage };
    EventHandler onWindowUpdate = { "WindowUpdate", this, &KeyboardBuffer::OnWindowUpdate };

  public: // methods

    KeyboardBuffer()
    {
      previousBuffer = &buffers[0];
      currentBuffer = &buffers[1];
    }

    // The button is down this frame and last frame.
    bool IsHeld(uint8_t vkKeyCode) const
    {
      return Current(vkKeyCode) && Previous(vkKeyCode);
    }

    // The button is up this frame but down last frame.
    bool IsReleased(uint8_t vkKeyCode) const
    {
      return !Current(vkKeyCode) && Previous(vkKeyCode);
    }

    // The button is down this frame but up last frame.
    bool IsTriggered(uint8_t vkKeyCode) const
    {
      return Current(vkKeyCode) && !Previous(vkKeyCode);
    }

    // The button is up this frame and last frame.
    bool IsUp(uint8_t vkKeyCode) const
    {
      return !Current(vkKeyCode) && !Previous(vkKeyCode);
    }

  private: // methods

    // Returns a reference to the current key value at the given index.
    bitset<maxKeys>::reference Current(size_t index) const
    {
      FatalIf(index >= maxKeys, "Key index out of range");
      return (*currentBuffer)[index];
    }

    // Handles key-down and key-up messages.
    void OnWindowMessage(EventData& eventData)
    {
      switch (eventData.Get<UINT>("uMsg"))
      {
      case WM_KEYDOWN:
        Current(eventData.Get<WPARAM>("wParam")) = true;
        break;
      case WM_KEYUP:
        Current(eventData.Get<WPARAM>("wParam")) = false;
        break;
      default:
        return;
      }

      eventData["handled"] = true;
    }

    // Sets the previous-frame buffer as the new current-frame buffer.
    void OnWindowUpdate(EventData&)
    {
      // Copy last frame's data into the previous buffer.
      *previousBuffer = *currentBuffer;
    }

    // Returns a reference to the previous key value at the given index.
    bitset<maxKeys>::reference Previous(size_t index) const
    {
      FatalIf(index >= maxKeys, "Key index out of range");
      return (*previousBuffer)[index];
    }
  };
} // namespace lite