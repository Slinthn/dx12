#pragma warning(push, 0)
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <math.h>
#include <hidusage.h>
#pragma warning(pop)

#include "win64_types.h"
#include "win64_math.c"
#include "resources.h"

#include "win64_rawinput.c"
#include "win64_dx12.c"

LRESULT WMessageProc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam) {
  WSTATE *state = (WSTATE *)GetWindowLongPtrA(window, GWLP_USERDATA);

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

WRESOURCE WLoadResource(unsigned int name, unsigned int type) {
  HRSRC src = FindResource(0, MAKEINTRESOURCE(name), MAKEINTRESOURCE(type));
  HGLOBAL global = LoadResource(0, src);
  void *data = LockResource(global);
  unsigned int size = SizeofResource(0, src);

  WRESOURCE res = {0};
  res.src = src;
  res.global = global;
  res.data = data;
  res.size = size;
  return res;
}

SM WLoadSM(WRESOURCE res) {
  SMHEADER *header = (SMHEADER *)res.data;
  if (CompareStringA(LOCALE_CUSTOM_DEFAULT, 0, header->signature, 2, "SM", 2) != CSTR_EQUAL) { // TODO does this work?
    ExitProcess(EXIT_ERROR_CODE_INVALID_SM);
  }

  SM ret = {0};
  ret.header = *header;
  ret.vertices = (VERTEX *)((unsigned long long)res.data + (unsigned long long)sizeof(SMHEADER));
  ret.indices = (unsigned int *)((unsigned long long)ret.vertices + (unsigned long long)(ret.header.vertexcount * sizeof(VERTEX)));
  return ret;
}

void WEntry(void) {
  HINSTANCE instance = GetModuleHandle(0);
  
  WNDCLASSEXA wc = {0};
  wc.cbSize = sizeof(wc);
  wc.hInstance = instance;
  wc.lpfnWndProc = WMessageProc;
  wc.lpszClassName = "24/06/2022Slinapp";

  if (!RegisterClassExA(&wc))
    ExitProcess(1);
 
  WSTATE wstate = {0}; 
  HWND window = CreateWindowExA(0, wc.lpszClassName, "App", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, &wstate);
  if (!window)
    ExitProcess(1);
  
  RIInit(window);
  DX12STATE state = DXInit(window);

  WRESOURCE cuberes = WLoadResource(CUBE, MODEL);
  SM cubesm = WLoadSM(cuberes);

  DX12VERTEXBUFFER vb = DXCreateVertexBuffer(&state, cubesm.vertices, sizeof(VERTEX), cubesm.header.vertexcount * sizeof(VERTEX));
  DX12INDEXBUFFER ib = DXCreateIndexBuffer(&state, cubesm.indices, cubesm.header.facecount * 3 * sizeof(unsigned int));

  WRESOURCE vertex = WLoadResource(DEFAULT_VERTEX, VERTEXSHADER);
  WRESOURCE pixel = WLoadResource(DEFAULT_PIXEL, PIXELSHADER);
  
  D3D12_INPUT_ELEMENT_DESC ieds[] =
  {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  };

  unsigned int cbsizes[] = {sizeof(CB0)};
  DXSHADER shader = DXCreateShader(&state, 1, cbsizes, vertex.data, vertex.size, pixel.data, pixel.size, ieds, SizeofArray(ieds));

  unsigned long long counter;
  unsigned long long frequency;
  QueryPerformanceCounter((LARGE_INTEGER *)&counter);
  QueryPerformanceFrequency((LARGE_INTEGER *)&frequency);

  GAMESTATE gstate = {0};
  
  while (1) {
    MSG msg;
    while (PeekMessageA(&msg, window, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessageA(&msg);
    }

    state.allocator->lpVtbl->Reset(state.allocator);
    state.list->lpVtbl->Reset(state.list, state.allocator, shader.pipeline);
    state.list->lpVtbl->SetGraphicsRootSignature(state.list, shader.rootsignature);
    state.list->lpVtbl->SetDescriptorHeaps(state.list, 1, &shader.cbvheap);
    state.list->lpVtbl->SetGraphicsRootDescriptorTable(state.list, 0, DXGetGPUDescriptorHandleForHeapStart(shader.cbvheap));

    DXPrepareFrame(&state);

    gstate.camera.position[0] += wstate.controls.move[0];
    gstate.camera.position[2] += wstate.controls.move[1];

    static unsigned int countera = 0;
    countera++;

    CB0 data = {0};
    MPerspective(&data.perspective, DToR(100.0f), 0.1f, 100.0f);
    float rot = DToR(countera / 5.0f);
    MTransform(&data.transform, 0, 0, 5, rot, rot, rot);
    MTransform(&data.camera, gstate.camera.position[0], gstate.camera.position[1], gstate.camera.position[2], gstate.camera.rotation[0], gstate.camera.rotation[1], gstate.camera.rotation[2]);

    CopyMemory(shader.cbptrs[0], &data, sizeof(CB0));
      
    state.list->lpVtbl->IASetPrimitiveTopology(state.list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    state.list->lpVtbl->IASetVertexBuffers(state.list, 0, 1, &vb.view);
    state.list->lpVtbl->IASetIndexBuffer(state.list, &ib.view);
    state.list->lpVtbl->DrawIndexedInstanced(state.list, cubesm.header.facecount * 3, 1, 0, 0, 0);

    DXFlushFrame(&state);

    int tosleep;
    do {
      unsigned long long newcounter;
      QueryPerformanceCounter((LARGE_INTEGER *)&newcounter);
      float delta = ((float)(newcounter - counter) / (float)frequency) * 1000.0f;
      tosleep = (int)floorf((1000.0f / 60.0f) - delta);
    } while (tosleep > 0);

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
  }
}
