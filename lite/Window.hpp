#pragma once

#include "EventHandler.hpp"
#include "Logging.hpp"
#include "Scripting.hpp"
#include "WindowsInclude.h"

namespace lite
{
  // Manages a Win32 window.
  class Window
  {
  private: // data

    int height;
    string title;
    int width;

    // Event handler for when the GetWindowInfo event occurs.
    EventHandler onWindowInfo = { "GetWindowInfo", this, &Window::OnWindowInfo };

    // Event handler for when the WindowMessage event occurs.
    EventHandler onWindowMessage = { "WindowMessage", this, &Window::OnWindowMessage };

  public: // properties

    HWND Handle = nullptr;
    const int& Height = height;
    const string& Title = title;
    const int& Width = width;

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
      SetWindowLongPtr(Handle, GWL_USERDATA, (LONG)this);

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

    // Sets the title of the window.
    void SetTitle(string title_)
    {
      title = move(title_);
      SetWindowTextA(Handle, title.c_str());
    }

    // Updates the Windows message pump.
    void Update()
    {
      InvokeEvent("WindowUpdate");

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

    // Event handler called when a GetWindowInfo event is invoked.
    void OnWindowInfo(EventData& data)
    {
      data["Handle"] = Handle;
      data["Width"] = Width;
      data["Height"] = Height;
      data["Title"] = Title;
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