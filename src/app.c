#pragma warning(push, 0)
#pragma warning(disable : 5045)
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <math.h>
#include <hidusage.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#pragma warning(pop)

#include "types.h"
#include "math/math.h"
#include "dx12.h"
#include "rawinput.h"
#include "app.h"
#include "resources.h"

#include "math/math.c"
#include "rawinput.c"
#include "dx12.c"
#include "resource.c"
#include "game.c"

LRESULT WMessageProc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam) {
  // Get windows state
  WINSTATE *state = (WINSTATE *)GetWindowLongPtrA(window, GWLP_USERDATA);

  switch (msg) {
  case WM_CREATE: {
    // Set the windows state variable pointer as userdata in the window
    SetWindowLongPtrA(window, GWLP_USERDATA, (LONG_PTR)(((CREATESTRUCT *)lparam)->lpCreateParams));
    return 1;
  } break;

  case WM_CLOSE:
  case WM_DESTROY: {
    // Exit the program if something happens to the window
    ExitProcess(0);
  } break;

  case WM_INPUT: {
    // Parse any raw input
    RIParse(&state->controls, (HRAWINPUT)lparam);
    return 1;
  } break;
  }
  
  return DefWindowProcA(window, msg, wparam, lparam);
}

int APIENTRY WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmd, int show) {
  // Suppress unused parameter warnings
  (void)prevInstance;
  (void)cmd;
  (void)show;
  
  // TODO is this a good idea
  timeBeginPeriod(1);

  // Create a windows state variable
  WINSTATE winstate = {0};

  // Register window class
  WNDCLASSEXA wc = {0};
  wc.cbSize = sizeof(WNDCLASSEXA);
  wc.hInstance = instance;
  wc.lpfnWndProc = WMessageProc;
  wc.lpszClassName = "24/06/2022Slinapp";

  RegisterClassExA(&wc);

  // Create window
  HWND window = CreateWindowExA(0, wc.lpszClassName, "App", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, &winstate);

  // Initialise raw input
  RIInit(window);

  // Initialise directx12
  winstate.dxstate = DXInit(window);
  
  // Invoke game initialiser function
  GameInit(&winstate);
  
  // Initialise FPS manager
  UQWORD counter;
  UQWORD frequency;
  QueryPerformanceCounter((LARGE_INTEGER *)&counter);
  QueryPerformanceFrequency((LARGE_INTEGER *)&frequency);
  
  while (1) {
    // Parse window messages
    MSG msg;
    while (PeekMessageA(&msg, window, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessageA(&msg);
    }

    // Update game
    GameUpdate(&winstate);

    // Sleep if time is remaining
    // TODO is Sleep() a good idea
    SDWORD tosleep;
    do {
      UQWORD newcounter;
      QueryPerformanceCounter((LARGE_INTEGER *)&newcounter);
      float delta = ((float)(newcounter - counter) / (float)frequency) * 1000.0f;
      tosleep = (SDWORD)floorf((1000.0f / 60.0f) - delta);
      if (tosleep > 0)
        Sleep(tosleep);
    } while (tosleep > 0);

    // Update counter
    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
  }
}
