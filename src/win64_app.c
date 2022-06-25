#pragma warning(push, 0)
#include <windows.h>
#include <dxgi.h>
#include <d3d12.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>
#include <dxgi1_4.h>
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

typedef struct {
  ID3D12Debug *debug;
  IDXGIFactory4 *factory;
  IDXGIAdapter *adapter;
  ID3D12Device *device;
  ID3D12InfoQueue *infoqueue;
  ID3D12CommandQueue *queue;
  IDXGISwapChain1 *swapchain;
  ID3D12DescriptorHeap *heap;
  unsigned long long rtvDescriptorSize;
  ID3D12Resource *rendertargets[2];
  ID3D12CommandAllocator *allocator;
  ID3D12GraphicsCommandList *list;
  ID3D12Fence *fence;
  HANDLE fenceevent;
  unsigned long long fencevalue;
} DX12STATE;

void DxWaitForFence(DX12STATE *state) {
  state->queue->lpVtbl->Signal(state->queue, state->fence, state->fencevalue);
  if (state->fence->lpVtbl->GetCompletedValue(state->fence) < state->fencevalue) {
    state->fence->lpVtbl->SetEventOnCompletion(state->fence, state->fencevalue, state->fenceevent);
    WaitForSingleObject(state->fenceevent, INFINITE);
  }

  state->fencevalue++;
}

DX12STATE DxInit(HWND window) {
  ID3D12Debug *debug;
#ifdef SLINAPP_DEBUG
  D3D12GetDebugInterface(&IID_ID3D12Debug, &debug);
  if (debug) {
    debug->lpVtbl->EnableDebugLayer(debug);
  }
#else
  debug = 0;
#endif

#ifdef SLINAPP_DEBUG
  unsigned int flags = DXGI_CREATE_FACTORY_DEBUG;
#else
  unsigned int flags = 0;
#endif

  IDXGIFactory4 *factory;
  CreateDXGIFactory2(flags, &IID_IDXGIFactory, &factory);

  IDXGIAdapter *adapter;
  ID3D12Device *device;
#if 0
  for (int i = 0; factory->lpVtbl->EnumAdapters(factory, i, &adapter) != DXGI_ERROR_NOT_FOUND; i++) {
    if (D3D12CreateDevice((IUnknown *)adapter, D3D_FEATURE_LEVEL_12_0, &IID_ID3D12Device, 0) == S_FALSE) {
      break;
    }
  }

#else

  factory->lpVtbl->EnumWarpAdapter(factory, &IID_IDXGIAdapter, &adapter);

#endif

  D3D12CreateDevice((IUnknown *)adapter, D3D_FEATURE_LEVEL_12_0, &IID_ID3D12Device, &device);

  ID3D12InfoQueue *infoqueue;
#ifdef SLINAPP_DEBUG
  device->lpVtbl->QueryInterface(device, &IID_ID3D12InfoQueue, &infoqueue);
  infoqueue->lpVtbl->SetBreakOnSeverity(infoqueue, D3D12_MESSAGE_SEVERITY_CORRUPTION, 1);
  infoqueue->lpVtbl->SetBreakOnSeverity(infoqueue, D3D12_MESSAGE_SEVERITY_WARNING, 1);
  infoqueue->lpVtbl->SetBreakOnSeverity(infoqueue, D3D12_MESSAGE_SEVERITY_ERROR, 1);
#else
  infoqueue = 0;
#endif

  D3D12_COMMAND_QUEUE_DESC cqd = {0};
  ID3D12CommandQueue *queue;
  device->lpVtbl->CreateCommandQueue(device, &cqd, &IID_ID3D12CommandQueue, &queue);

  DXGI_SWAP_CHAIN_DESC1 scd = {0};
  scd.BufferCount = 2;
  scd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  scd.SampleDesc.Count = 1;
  IDXGISwapChain1 *swapchain;
  factory->lpVtbl->CreateSwapChainForHwnd(factory, (IUnknown *)queue, window, &scd, 0, 0, &swapchain);

  D3D12_DESCRIPTOR_HEAP_DESC dhd = {0};
  dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  dhd.NumDescriptors = 2;
  ID3D12DescriptorHeap *heap;
  device->lpVtbl->CreateDescriptorHeap(device, &dhd, &IID_ID3D12DescriptorHeap, &heap);

#pragma warning(push) 
#pragma warning(disable : 4020) 
  D3D12_CPU_DESCRIPTOR_HANDLE cpudescriptor;
  heap->lpVtbl->GetCPUDescriptorHandleForHeapStart(heap, &cpudescriptor);
#pragma warning(pop)

  unsigned int rtvDescriptorSize = device->lpVtbl->GetDescriptorHandleIncrementSize(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  ID3D12Resource *rendertargets[2];
  for (unsigned int i = 0; i < 2; i++) {
    swapchain->lpVtbl->GetBuffer(swapchain, i, &IID_ID3D12Resource, &rendertargets[i]);
    device->lpVtbl->CreateRenderTargetView(device, rendertargets[i], 0, cpudescriptor);
    cpudescriptor.ptr += rtvDescriptorSize;
  }

  ID3D12CommandAllocator *allocator;
  ID3D12GraphicsCommandList *list;
  ID3D12Fence *fence;
  device->lpVtbl->CreateCommandAllocator(device, 0, &IID_ID3D12CommandAllocator, &allocator);
  device->lpVtbl->CreateCommandList(device, 0, 0, allocator, 0, &IID_ID3D12CommandList, &list);
  device->lpVtbl->CreateFence(device, 0, 0, &IID_ID3D12Fence, &fence);

  HANDLE fenceevent = CreateEvent(0, 0, 0, 0);

  DX12STATE state = {0};
  state.debug = debug;
  state.factory = factory;
  state.adapter = adapter;
  state.device = device;
  state.queue = queue;
  state.infoqueue = infoqueue;
  state.swapchain = swapchain;
  state.heap = heap;
  state.rtvDescriptorSize = rtvDescriptorSize;
  state.rendertargets[0] = rendertargets[0];
  state.rendertargets[1] = rendertargets[1];
  state.allocator = allocator;
  state.list = list;
  state.fence = fence;
  state.fenceevent = fenceevent;

  DxWaitForFence(&state);

  D3D12_VIEWPORT viewport = {0, 0, 500, 500};
  list->lpVtbl->RSSetViewports(list, 1, &viewport);

  list->lpVtbl->IASetPrimitiveTopology(list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  list->lpVtbl->Close(list);
  queue->lpVtbl->ExecuteCommandLists(queue, 1, (ID3D12CommandList **)&list);
  DxWaitForFence(&state);

  return state;
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
  
  DX12STATE state = DxInit(window);

  unsigned int alternatingframe = 0;
  state.list->lpVtbl->Reset(state.list, state.allocator, 0);

  while (1) {
    MSG msg;
    while (PeekMessageA(&msg, window, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessageA(&msg);
    }

#if 0
  D3D12_RESOURCE_BARRIER rb = {0};
  rb.Transition.pResource = state.rendertargets[alternatingframe];
  rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  rb.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
  state.list->lpVtbl->ResourceBarrier(state.list, 1, &rb);

  D3D12_CPU_DESCRIPTOR_HANDLE cpudescriptor;
  state.heap->lpVtbl->GetCPUDescriptorHandleForHeapStart(state.heap, &cpudescriptor);
  cpudescriptor.ptr += alternatingframe * state.rtvDescriptorSize;
  state.list->lpVtbl->OMSetRenderTargets(state.list, 1, &cpudescriptor, 0, 0);

    
  state.list->lpVtbl->ClearRenderTargetView(state.list, cpudescriptor, (float[4]) {alternatingframe, 0, 0, 1.0f}, 0, 0);

  rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  rb.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
  rb.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
  state.list->lpVtbl->ResourceBarrier(state.list, 1, &rb);
#endif
    
  state.list->lpVtbl->Close(state.list);
  state.queue->lpVtbl->ExecuteCommandLists(state.queue, 1, (ID3D12CommandList **)&state.list);
  state.list->lpVtbl->Reset(state.list, state.allocator, 0);
  //DxWaitForFence(&state);
    state.swapchain->lpVtbl->Present(state.swapchain, 1, 0);
    alternatingframe = (alternatingframe + 1) % 2;
  }
}
