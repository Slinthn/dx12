#pragma warning(push, 0)
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <math.h>
#include <hidusage.h>
#pragma warning(pop)

#include "header/win64_types.h"
#include "header/win64_math.h"
#include "header/win64_dx12.h"
#include "header/win64_resource.h"
#include "header/win64_rawinput.h"
#include "header/win64_app.h"
#include "header/win64_resources.h"

#include "win64_math.c"
#include "win64_rawinput.c"
#include "win64_dx12.c"
#include "win64_resource.c"

LRESULT WMessageProc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam) {
  WINSTATE *state = (WINSTATE *)GetWindowLongPtrA(window, GWLP_USERDATA);

  switch (msg) {
  case WM_CREATE: {
    SetWindowLongPtrA(window, GWLP_USERDATA, (LONG_PTR)(((CREATESTRUCT *)lparam)->lpCreateParams));
    return 1;
  } break;

  case WM_CLOSE:
  case WM_DESTROY: {
    ExitProcess(0);
  } break;

  case WM_INPUT: {
    RIParse(&state->controls, (HRAWINPUT)lparam);
    return DefWindowProcA(window, msg, wparam, lparam);
  } break;
  }
  
  return DefWindowProcA(window, msg, wparam, lparam);
}

#include "win64_game.c"

void WEntry(void) {
  HINSTANCE instance = GetModuleHandle(0);
  
  WNDCLASSEXA wc = {0};
  wc.cbSize = sizeof(WNDCLASSEXA);
  wc.hInstance = instance;
  wc.lpfnWndProc = WMessageProc;
  wc.lpszClassName = "24/06/2022Slinapp";

  if (!RegisterClassExA(&wc))
    ExitProcess(1);
 
  WINSTATE winstate = {0};

  HWND window = CreateWindowExA(0, wc.lpszClassName, "App", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, &winstate);
  if (!window)
    ExitProcess(1);
  
  RIInit(window);
  winstate.dxstate = DXInit(window);

  GameInit(&winstate);

  UQWORD counter;
  UQWORD frequency;
  QueryPerformanceCounter((LARGE_INTEGER *)&counter);
  QueryPerformanceFrequency((LARGE_INTEGER *)&frequency);
  
  while (1) {
    MSG msg;
    while (PeekMessageA(&msg, window, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessageA(&msg);
    }

    GameUpdate(&winstate);

    SDWORD tosleep;
    do {
      UQWORD newcounter;
      QueryPerformanceCounter((LARGE_INTEGER *)&newcounter);
      float delta = ((float)(newcounter - counter) / (float)frequency) * 1000.0f;
      tosleep = (int)floorf((1000.0f / 60.0f) - delta);
    } while (tosleep > 0);

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
  }
}
