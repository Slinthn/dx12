#pragma warning(push, 0)
#pragma warning(disable : 5045)
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <math.h>
#include <hidusage.h>

#define STB_IMAGE_IMPLEMENTATION
#include "include/stb_image.h"
#define CGLTF_IMPLEMENTATION
#include "include/cgltf.h"
#pragma warning(pop)

#include "include/types.h"

#include "math/math.c"
#include "rawinput/rawinput.c"
#include "renderer/dx12.c"
#include "resource.c"
#include "game.c"

LRESULT win64_message_proc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
{
  // Get windows state
  struct win64_state *state =
    (struct win64_state *)GetWindowLongPtrA(window, GWLP_USERDATA);

  switch (msg) {
  case WM_CREATE: {
    // Set the windows state variable pointer as userdata in the window
    SetWindowLongPtrA(window, GWLP_USERDATA,
      (LONG_PTR)(((CREATESTRUCT *)lparam)->lpCreateParams));
    return 1;
  } break;

  case WM_CLOSE:
  case WM_DESTROY: {
    ExitProcess(0);
  } break;

  case WM_INPUT: {
    rawinput_parse(&state->controls, (HRAWINPUT)lparam);
    return 1;
  } break;
  }
  
  return DefWindowProcA(window, msg, wparam, lparam);
}

int APIENTRY WinMain(HINSTANCE instance,
  HINSTANCE prevInstance, LPSTR cmd, int show)
{
  // Suppress unused parameter warnings
  (void)prevInstance;
  (void)cmd;
  (void)show;

  // Register window class
  WNDCLASSEXA wc = {0};
  wc.cbSize = sizeof(wc);
  wc.hInstance = instance;
  wc.lpfnWndProc = win64_message_proc;
  wc.lpszClassName = "24/06/2022Slinapp";

  RegisterClassExA(&wc);

  struct win64_state winstate = {0};

  HWND window = CreateWindowExA(0, wc.lpszClassName, "App",
    WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
    CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, &winstate);

  rawinput_init(window);

  winstate.dxstate = dx12_init(window, 1);
  
  game_init(&winstate);
  
  uint64_t counter;
  uint64_t frequency;
  QueryPerformanceCounter((LARGE_INTEGER *)&counter);
  QueryPerformanceFrequency((LARGE_INTEGER *)&frequency);

  while (1) {
    MSG msg;
    while (PeekMessageA(&msg, window, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessageA(&msg);
    }

    game_update(&winstate);

    // Sleep if time is remaining
    int32_t tosleep;
    do {
      uint64_t newcounter;
      QueryPerformanceCounter((LARGE_INTEGER *)&newcounter);
      float delta_seconds = ((newcounter - counter) / (float)frequency);
      tosleep = (int32_t)floorf((1 / 60.0f - delta_seconds) * 1000.0f);
    } while (tosleep > 0);

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
  }
}
