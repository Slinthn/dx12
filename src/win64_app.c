#pragma warning(push, 0)
#include <windows.h>
#include <dxgi.h>
#include <d3d12.h>
#pragma warning(pop)

extern const IID IID_ID3D12Debug;
extern const IID IID_ID3D12Device;
extern const IID IID_ID3D12CommandQueue;
extern const IID IID_IDXGIFactory;
extern const IID IID_ID3D12DescriptorHeap;
extern const IID IID_ID3D12Resource;

LRESULT WinMessageProc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam) {
  switch (msg) {
  case WM_CLOSE:
  case WM_DESTROY: {
    ExitProcess(0);
  } break;

  default: {
    return DefWindowProc(window, msg, wparam, lparam);
  } break;
  }
}

void DxInit(HWND window) {
  ID3D12Debug *debug;
  D3D12GetDebugInterface(&IID_ID3D12Debug, &debug);
  if (debug) {
    debug->lpVtbl->EnableDebugLayer(debug);
  }

  IDXGIFactory *factory;
  CreateDXGIFactory1(&IID_IDXGIFactory, &factory);

  IDXGIAdapter *adapter;
  ID3D12Device *device;
  for (int i = 0; factory->lpVtbl->EnumAdapters(factory, i, &adapter) != DXGI_ERROR_NOT_FOUND; i++) {
    if (D3D12CreateDevice(0, D3D_FEATURE_LEVEL_10_0, &IID_ID3D12Device, 0) == S_FALSE) {
      break;
    }
  }
  D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_10_0, &IID_ID3D12Device, &device);

  D3D12_COMMAND_QUEUE_DESC cqd = {0};
  ID3D12CommandQueue *queue;
  device->lpVtbl->CreateCommandQueue(device, &cqd, &IID_ID3D12CommandQueue, &queue);

  DXGI_SWAP_CHAIN_DESC scd = {0};
  scd.BufferCount = 2;
  scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  scd.BufferUsage = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  scd.OutputWindow = window;
  scd.SampleDesc.Count = 1;
  scd.Windowed = 1;
  IDXGISwapChain *swapchain;
  factory->lpVtbl->CreateSwapChain(factory, queue, &scd, &swapchain);

  D3D12_DESCRIPTOR_HEAP_DESC dhd = {0};
  dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  dhd.NumDescriptors = 2;
  ID3D12DescriptorHeap *heap;
  device->lpVtbl->CreateDescriptorHeap(device, &dhd, &IID_ID3D12DescriptorHeap, &heap);
  
  D3D12_CPU_DESCRIPTOR_HANDLE cpudescriptor = heap->lpVtbl->GetCPUDescriptorHandleForHeapStart(heap);

  for (int i = 0; i < 2; i++) {
    ID3D12Resource *rendertarget;
    swapchain->lpVtbl->GetBuffer(swapchain, i, &IID_ID3D12Resource, &rendertarget);
    device->lpVtbl->CreateRenderTargetView(device, rendertarget, 0, cpudescriptor);
  }
}

void MainEntry(void) {
  HINSTANCE instance = GetModuleHandle(0);

  WNDCLASSEXA wc = {0};
  wc.cbSize = sizeof(wc);
  wc.hInstance = instance;
  wc.lpfnWndProc = WinMessageProc;
  wc.lpszClassName = "24/06/2022";

  if (!RegisterClassExA(&wc))
    ExitProcess(1);
  
  HWND window = CreateWindowExA(0, wc.lpszClassName, "App", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, 0);
  if (!window)
    ExitProcess(1);
  
  DxInit(window);

  while (1) {
    MSG msg;
    while (PeekMessageA(&msg, window, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessageA(&msg);
    }
  }
}
