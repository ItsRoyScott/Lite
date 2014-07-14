#pragma once

#include "EventHandler.hpp"
#include "Logging.hpp"
#include "WindowsInclude.h"

namespace lite
{
  // Manages a Win32 window.
  class Window
  {
  private: // data

    bool cursorWasClipped = false;
    int height;
    string title;
    int width;

    // Event handler for when the GetWindowInfo event occurs.
    EventHandler onWindowInfo = { "GetWindowInfo", this, &Window::OnWindowInfo };

    // Event handler for when the WindowMessage event occurs.
    EventHandler onWindowMessage = { "WindowMessage", this, &Window::OnWindowMessage };

  public: // data

    // Whether the cursor will be wrapped within the window rectangle.
    bool ClipCursor = true;

    // Win32 window handle.
    HWND Handle = nullptr;

    // Height of the window.
    const int& Height = height;

    // Width of the window.
    const int& Width = width;

  public: // properties

    // Whether the cursor was clipped to the other side of the window this frame.
    const bool& CursorWasClipped() const { return cursorWasClipped; }

    // Title of the window.
    const string& Title() const { return title; }
    void Title(string title_) 
    { 
      title = move(title_); 
      SetWindowTextA(Handle, title.c_str());
    }

  public: // methods

    // Creates the window with an initial title, width, and height.
    explicit Window(string title_ = "Untitled", int width_ = 960, int height_ = 540) :
      height(height_),
      title(move(title_)),
      width(width_)
    {
      // Get a handle to this .exe file.
      HINSTANCE hInstance = (HINSTANCE) GetModuleHandle(NULL);

      // Fill in the window class information struct.
      WNDCLASSEX wclass;
      wclass.cbSize = sizeof WNDCLASSEX;
      wclass.style = CS_HREDRAW | CS_VREDRAW;
      wclass.lpfnWndProc = StaticWindowProc;
      wclass.cbClsExtra = 0;
      wclass.cbWndExtra = 0;
      wclass.hInstance = hInstance;
      wclass.hIcon = LoadIcon(hInstance, IDI_WINLOGO);
      wclass.hIconSm = NULL;
      wclass.hCursor = LoadCursor(NULL, IDC_ARROW);
      wclass.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
      wclass.lpszMenuName = NULL;
      wclass.lpszClassName = title.c_str();

      // Register the class with Win32.
      if (!RegisterClassEx(&wclass)) throw runtime_error("Window class could not be created for " + title);

      // Create the window.
      Handle = CreateWindowA(
        wclass.lpszClassName,   // class name
        title.c_str(),          // window title
        WS_OVERLAPPEDWINDOW,    // window style
        CW_USEDEFAULT,          // x
        CW_USEDEFAULT,          // y
        width,                  // width
        height,                 // height
        nullptr,                // parent handle
        nullptr,                // menu handle
        wclass.hInstance,       // handle to the process instance
        nullptr);               // optional context pointer

      if (!Handle) throw runtime_error("Window could not be created");

      // Set a user data pointer so we can retrieve 'this' inside the WindowProc callback.
      SetWindowLongPtr(Handle, GWLP_USERDATA, (LONG_PTR)this);

      // Present the window to the user.
      ShowWindow(Handle, SW_SHOWDEFAULT);
    }

    Window(Window&& b) :
      height(b.height),
      title(move(b.title)),
      width(b.width),
      Handle(b.Handle)
    {
      b.Handle = nullptr;

      if (Handle)
      {
        SetWindowLongPtr(Handle, GWLP_USERDATA, (LONG_PTR)this);
      }
    }

    // Destroys the window.
    ~Window()
    {
      if (Handle)
      {
        DestroyWindow(Handle);
        Handle = nullptr;
      }
    }

    Window& operator=(Window&& b)
    {
      Handle = nullptr;

      height = b.height;
      title = move(b.title);
      width = b.width;
      Handle = b.Handle;

      b.Handle = nullptr;

      if (Handle)
      {
        SetWindowLongPtr(Handle, GWLP_USERDATA, (LONG_PTR)this);
      }

      return *this;
    }

    // Closes the window.
    void Close()
    {
      CloseWindow(Handle);
    }

    // Returns whether the window is open.
    bool IsOpen() const
    {
      return Handle != nullptr;
    }

    // Updates the Windows message pump.
    void Update()
    {
      // Broadcast a window-update event.
      InvokeEvent("WindowUpdate");

      // Clip the cursor.
      if (ClipCursor)
      {
        ClipCursorInWindow();
      }

      // Process all messages in the process and window message pump.
      // This will call our WindowProc callback.
      MSG msg = { 0 };
      while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
      {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
      }
    }

  private: // methods

    void ClipCursorInWindow()
    {
      cursorWasClipped = false;

      // Get the window rectangle and cursor position.
      RECT windowRect;
      GetWindowRect(Handle, &windowRect);
      POINT cursorPos;
      GetCursorPos(&cursorPos);

      // If the cursor is within the window:
      if (cursorPos.x >= windowRect.left &&
        cursorPos.x <= windowRect.right &&
        cursorPos.y >= windowRect.top &&
        cursorPos.y <= windowRect.bottom)
      {
        windowRect.left += 8;
        windowRect.right -= 8;
        windowRect.top += 8;
        windowRect.bottom -= 8;

        // Clip the cursor position within the window.
        POINT newCursorPos = cursorPos;
        if (cursorPos.x < windowRect.left)        newCursorPos.x = windowRect.right;
        else if (cursorPos.x > windowRect.right)  newCursorPos.x = windowRect.left;
        if (cursorPos.y < windowRect.top)         newCursorPos.y = windowRect.bottom;
        else if (cursorPos.y > windowRect.bottom) newCursorPos.y = windowRect.top;

        if (newCursorPos.x != cursorPos.x || newCursorPos.y != cursorPos.y)
        {
          cursorWasClipped = true;
          SetCursorPos(newCursorPos.x, newCursorPos.y);
        }
      }
    }

    // Event handler called when a GetWindowInfo event is invoked.
    void OnWindowInfo(EventData& data)
    {
      data["Handle"] = Handle;
      data["Width"] = Width;
      data["Height"] = Height;
      data["Title"] = title;
    }

    // Event handler called when a WindowMessage event is invoked.
    void OnWindowMessage(EventData& data)
    {
      switch (data.Get<unsigned>("uMsg"))
      {
      case WM_CLOSE: // 'X' on window has been pressed
        DestroyWindow(data.Get<HWND>("hWnd"));
        data["handled"] = true;
        break;

      case WM_DESTROY: // window is being destroyed
        Handle = nullptr;
        data["handled"] = true;
        break;

      case WM_PAINT: // request to paint the window
      {
        PAINTSTRUCT ps;
        BeginPaint(Handle, &ps);
        EndPaint(Handle, &ps);
        data["handled"] = true;
        break;
      }

      case WM_SYSKEYDOWN: // handle ALT press
        if (data.Get<WPARAM>("wParam") == VK_F4)
        {
          DestroyWindow(Handle);
        }
        data["handled"] = true;
        break;
      }
    }

    // Static function used by the Win32 API to handle window-related messages.
    static LRESULT CALLBACK StaticWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
      // Call a global event to handle all Window messages.
      EventData data;
      data["hWnd"] = hWnd;
      data["uMsg"] = uMsg;
      data["wParam"] = wParam;
      data["lParam"] = lParam;
      data["handled"] = false;
      InvokeEvent("WindowMessage", data);

      // Use the default message handler if the message was not handled.
      if (!data.Get<bool>("handled"))
      {
        return DefWindowProcA(hWnd, uMsg, wParam, lParam);
      }

      return 0;
    }
  };
} // namespace lite