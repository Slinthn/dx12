#pragma warning(push, 0)
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <math.h>
#include <hidusage.h>
#pragma warning(pop)

#include "win64_math.h"
#include "resources.h"

#define SizeofArray(x) ((sizeof(x) / sizeof((x)[0])))

#pragma pack(push, 1)
typedef struct {
  MATRIX perspective;
  MATRIX transform;
  MATRIX camera;
  MATRIX nothings[1];
} CB0;
#pragma pack(pop)
  

typedef struct {
  void *ptr;
  ID3D12Debug *debug;
  IDXGIFactory4 *factory;
  IDXGIAdapter *adapter;
  ID3D12Device *device;
  ID3D12InfoQueue *infoqueue;
  ID3D12CommandQueue *queue;
  IDXGISwapChain3 *swapchain;
  ID3D12DescriptorHeap *rtvheap;
  ID3D12DescriptorHeap *cbvheap;
  ID3D12Resource *rendertargets[2];
  ID3D12CommandAllocator *allocator;
  ID3D12GraphicsCommandList *list;
  ID3D12Fence *fence;
  HANDLE fenceevent;
  unsigned long long fencevalue;
  ID3D12RootSignature *rootsignature;
  ID3D12PipelineState *pipeline;
  ID3D12DescriptorHeap *dsvheap;
} DX12STATE;

typedef struct {
  ID3D12Resource *buffer;
  D3D12_VERTEX_BUFFER_VIEW view;
} DX12VERTEXBUFFER;

typedef struct {
  ID3D12Resource *buffer;
  D3D12_INDEX_BUFFER_VIEW view;
} DX12INDEXBUFFER;

#pragma pack(push, 1)
typedef struct {
  float position[3];
  float colour[4];
} VERTEX;
#pragma pack(pop)

#include "win64_rawinput.c"
#include "win64_dx12.c"

LRESULT WinMessageProc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam) {
  switch (msg) {
  case WM_CLOSE:
  case WM_DESTROY: {
    ExitProcess(0);
  } break;

  case WM_INPUT: {
    RIParse((HRAWINPUT)lparam);
  };

  default: {
    return DefWindowProc(window, msg, wparam, lparam);
  } break;
  }
}

void MainEntry(void) {
  HINSTANCE instance = GetModuleHandle(0);
  
  WNDCLASSEXA wc = {0};
  wc.cbSize = sizeof(wc);
  wc.hInstance = instance;
  wc.lpfnWndProc = WinMessageProc;
  wc.lpszClassName = "24/06/2022Slinapp";

  if (!RegisterClassExA(&wc))
    ExitProcess(1);
  
  HWND window = CreateWindowExA(0, wc.lpszClassName, "App", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, 0);
  if (!window)
    ExitProcess(1);
  
  RIInit(window);
  DX12STATE state = DXInit(window);

  VERTEX vertices[] =
  {
    {{-1, -1, -1}, {1, 0, 0, 1}},
    {{1, -1, -1},  {0, 1, 0, 1}},
    {{1, 1, -1},   {0, 0, 1, 1}},
    {{-1, 1, -1},  {0, 1, 0, 1}},
    {{-1, -1, 1}, {0, 0, 1, 1}},
    {{1, -1, 1},  {0, 1, 0, 1}},
    {{1, 1, 1},   {1, 0, 0, 1}},
    {{-1, 1, 1},  {0, 1, 0, 1}},
  };
  DX12VERTEXBUFFER vb = DXCreateVertexBuffer(&state, vertices, sizeof(VERTEX), sizeof(vertices));

  unsigned int indices[] = {
  		// front
		0, 1, 2,
		2, 3, 0,
		// right
		1, 5, 6,
		6, 2, 1,
		// back
		7, 6, 5,
		5, 4, 7,
		// left
		4, 0, 3,
		3, 7, 4,
		// bottom
		4, 5, 1,
		1, 0, 4,
		// top
		3, 2, 6,
		6, 7, 3
  
  };
  DX12INDEXBUFFER ib = DXCreateIndexBuffer(&state, indices, sizeof(indices));

  unsigned long long counter;
  unsigned long long frequency;
  QueryPerformanceCounter((LARGE_INTEGER *)&counter);
  QueryPerformanceFrequency((LARGE_INTEGER *)&frequency);

  while (1) {
    MSG msg;
    while (PeekMessageA(&msg, window, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessageA(&msg);
    }

    DXPrepareFrame(&state);

    static unsigned int countera = 0;
    countera++;

    CB0 data = {0};
    MPerspective(&data.perspective, DToR(100.0f), 0.1f, 100.0f);
    float rot = DToR(countera / 5.0f);
    MTransform(&data.transform, 0, 0, 5, rot, rot, rot);
    MTransform(&data.camera, 0, 0, -rot, 0, 0, 0);

    CopyMemory(state.ptr, &data, sizeof(CB0));
      
    state.list->lpVtbl->IASetPrimitiveTopology(state.list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    state.list->lpVtbl->IASetVertexBuffers(state.list, 0, 1, &vb.view);
    state.list->lpVtbl->IASetIndexBuffer(state.list, &ib.view);
    state.list->lpVtbl->DrawIndexedInstanced(state.list, 3*12, 1, 0, 0, 0);

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
