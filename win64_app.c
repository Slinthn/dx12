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

#include "win64_types.h"
#include "win64_math.h"
#include "win64_dx12.h"
#include "win64_rawinput.h"
#include "win64_app.h"
#include "win64_resources.h"

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

//void __scrt_initialize_crt(void);

// TODO Debug
#if 0
void MemcpySubresource(
    const D3D12_MEMCPY_DEST* pDest,
    const D3D12_SUBRESOURCE_DATA* pSrc,
    SIZE_T RowSizeInBytes,
    UINT NumRows,
    UINT NumSlices)
{
    for (UINT z = 0; z < NumSlices; ++z)
    {
        BYTE *pDestSlice = (BYTE*)(pDest->pData) + pDest->SlicePitch * z;
        BYTE *pSrcSlice = (BYTE*)(pSrc->pData) + pSrc->SlicePitch * (LONG_PTR)(z);
        for (UINT y = 0; y < NumRows; ++y)
        {
            memcpy(pDestSlice + pDest->RowPitch * y,
                   pSrcSlice + pSrc->RowPitch * (LONG_PTR)(y),
                   RowSizeInBytes);
        }
    }
}
#endif

// TODO end debug

//void WEntry(void) {
#pragma warning(disable : 4100)
int WINAPI WinMain(HINSTANCE instance, HINSTANCE _, LPSTR __, int ___) {
#pragma warning(default : 4100)
  //__scrt_initialize_crt(); // NOTE I am absolutely FLABBERGASTED that this does not just crash the whole program and it actually works. (THIS LINE IS ONLY DUE TO STB_IMAGE.H)

  //HINSTANCE instance = GetModuleHandle(0);
  
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

  // TODO debug
#if 0
  WINRESOURCE tex = WINLoadResource(IMAGE, PNG);
  SDWORD width, height, n;

  void *imgdata = stbi_load_from_memory(tex.data, tex.size, &width, &height, &n, 0);

  D3D12_HEAP_PROPERTIES heapproperty = {0};
  heapproperty.Type = D3D12_HEAP_TYPE_DEFAULT;
  heapproperty.CreationNodeMask = 1;
  heapproperty.VisibleNodeMask = 1;

  D3D12_RESOURCE_DESC resourcedesc = {0};
  resourcedesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  resourcedesc.Width = width;
  resourcedesc.Height = height;
  resourcedesc.DepthOrArraySize = 1;
  resourcedesc.MipLevels = 1;
  resourcedesc.SampleDesc.Count = 1;
  resourcedesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

  ID3D12Resource *texturebuffer;
  winstate.dxstate.device->lpVtbl->CreateCommittedResource(winstate.dxstate.device, &heapproperty, 0, &resourcedesc, D3D12_RESOURCE_STATE_COPY_DEST, 0, &IID_ID3D12Resource, &texturebuffer);

  heapproperty = (D3D12_HEAP_PROPERTIES){0};
  heapproperty.Type = D3D12_HEAP_TYPE_UPLOAD;
  heapproperty.CreationNodeMask = 1;
  heapproperty.VisibleNodeMask = 1;

  UDWORD FirstSubresource = 0, NumSubresources = 1, pNumRows;
  UQWORD IntermediateOffset = 0, pRowSizesInBytes, RequiredSize;
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT pLayout;
  winstate.dxstate.device->lpVtbl->GetCopyableFootprints(winstate.dxstate.device, &resourcedesc, FirstSubresource, NumSubresources, IntermediateOffset, &pLayout, &pNumRows, &pRowSizesInBytes, &RequiredSize);

  resourcedesc = (D3D12_RESOURCE_DESC){0};
  resourcedesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourcedesc.Width = RequiredSize;
  resourcedesc.Height = 1;
  resourcedesc.DepthOrArraySize = 1;
  resourcedesc.MipLevels = 1;
  resourcedesc.SampleDesc.Count = 1;
  resourcedesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  ID3D12Resource *textureuploadbuffer;
  winstate.dxstate.device->lpVtbl->CreateCommittedResource(winstate.dxstate.device, &heapproperty, 0, &resourcedesc, D3D12_RESOURCE_STATE_GENERIC_READ, 0, &IID_ID3D12Resource, &textureuploadbuffer);

  void *ptr;
  D3D12_RANGE range = {0};
  textureuploadbuffer->lpVtbl->Map(textureuploadbuffer, 0, &range, &ptr);
  CopyMemory(ptr, imgdata, width * height * 4);
  textureuploadbuffer->lpVtbl->Unmap(textureuploadbuffer, 0, 0);

  D3D12_TEXTURE_COPY_LOCATION dst = {0};
  dst.pResource = texturebuffer;

  D3D12_TEXTURE_COPY_LOCATION src = {0};
  src.pResource = textureuploadbuffer;
  src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
  src.PlacedFootprint = pLayout;

  winstate.dxstate.allocator->lpVtbl->Reset(winstate.dxstate.allocator);
  winstate.dxstate.list->lpVtbl->Reset(winstate.dxstate.list, winstate.dxstate.allocator, 0);

  winstate.dxstate.list->lpVtbl->CopyTextureRegion(winstate.dxstate.list, &dst, 0, 0, 0, &src, 0);


  D3D12_SHADER_RESOURCE_VIEW_DESC srvdesc = {0};
  srvdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvdesc.Texture2D.MipLevels = 1;

#if 0
  D3D12_CPU_DESCRIPTOR_HANDLE dh = DXGetCPUDescriptorHandleForHeapStart(winstate.shader.cbvheap);
  dh.ptr += winstate.dxstate.device->lpVtbl->GetDescriptorHandleIncrementSize(winstate.dxstate.device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  winstate.dxstate.device->lpVtbl->CreateShaderResourceView(winstate.dxstate.device, texturebuffer, &srvdesc, dh);
#endif

  D3D12_RESOURCE_BARRIER rb = {0};
  rb.Transition.pResource = texturebuffer;
  rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  rb.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
  rb.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
  winstate.dxstate.list->lpVtbl->ResourceBarrier(winstate.dxstate.list, 1, &rb);

#endif
  D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {0};
  samplerHeapDesc.NumDescriptors = 1;
  samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
  samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

  ID3D12DescriptorHeap *samplerheap;
  winstate.dxstate.device->lpVtbl->CreateDescriptorHeap(winstate.dxstate.device, &samplerHeapDesc, &IID_ID3D12DescriptorHeap, &samplerheap);

  D3D12_SAMPLER_DESC samplerdesc = {0};
  samplerdesc.Filter = D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
  samplerdesc.AddressU = samplerdesc.AddressV = samplerdesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  samplerdesc.MaxLOD = D3D12_FLOAT32_MAX;
  samplerdesc.MaxAnisotropy = 1;
  samplerdesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;

  winstate.dxstate.device->lpVtbl->CreateSampler(winstate.dxstate.device, &samplerdesc, DXGetCPUDescriptorHandleForHeapStart(samplerheap));

#if 0
  winstate.dxstate.list->lpVtbl->Close(winstate.dxstate.list);
  winstate.dxstate.queue->lpVtbl->ExecuteCommandLists(winstate.dxstate.queue, 1, (ID3D12CommandList **)&winstate.dxstate.list);
  DXWaitForFence(&winstate.dxstate);
#endif
  winstate.dxstate.samplerheap = samplerheap;


  // TODO end debug

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
