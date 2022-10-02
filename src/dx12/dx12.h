#include "descriptorheap.h"

typedef struct t_DX12SHADER {
  ID3D12RootSignature *rootsignature;
  ID3D12PipelineState *pipeline;
} DX12SHADER;

typedef struct t_DX12TEXTURE {
  ID3D12Resource *texture, *uploadtexture;
} DX12TEXTURE;

typedef struct t_DX12SAMPLER {
  ID3D12DescriptorHeap *heap;
} DX12SAMPLER;

typedef struct t_DX12STATE {
  ID3D12Device *device;
  ID3D12CommandQueue *queue;
  IDXGISwapChain3 *swapchain;
  ID3D12DescriptorHeap *rendertargetviewdescriptorheap;
  ID3D12Resource *rendertargets[2];
  ID3D12CommandAllocator *allocator;
  ID3D12GraphicsCommandList *list;
  ID3D12Fence *fence;
  ID3D12DescriptorHeap *depthstencilviewdescriptorheap;
} DX12STATE;

typedef struct t_DX12BUFFER {
  ID3D12Resource *buffer, *bufferupload;
} DX12BUFFER;

typedef struct t_DX12VERTEXBUFFER {
  ID3D12Resource *buffer, *bufferupload;
  D3D12_VERTEX_BUFFER_VIEW view;
} DX12VERTEXBUFFER;

typedef struct t_DX12INDEXBUFFER {
  ID3D12Resource *buffer, *bufferupload;
  D3D12_INDEX_BUFFER_VIEW view;
} DX12INDEXBUFFER;

typedef struct t_DX12SHADOW {
  ID3D12Resource *depthresource;
  ID3D12DescriptorHeap *descriptorheap;
  DX12DESCRIPTORHANDLE texturehandle;
} DX12SHADOW;