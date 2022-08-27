typedef struct {
  ID3D12RootSignature *rootsignature;
  ID3D12PipelineState *pipeline;
} DX12SHADER;

typedef struct {
  ID3D12Resource *texture, *uploadtexture;
} DX12TEXTURE;

typedef struct {
  ID3D12DescriptorHeap *heap;
} DX12SAMPLER;

typedef struct {
  IDXGIAdapter *adapter;
  ID3D12Device *device;
  ID3D12CommandQueue *queue;
  IDXGISwapChain3 *swapchain;
  ID3D12DescriptorHeap *rendertargetviewdescriptorheap;
  ID3D12Resource *rendertargets[2];
  ID3D12CommandAllocator *allocator;
  ID3D12GraphicsCommandList *list;
  ID3D12Fence *fence;
  ID3D12DescriptorHeap *depthstencilviewheap;
} DX12STATE;

typedef struct {
  ID3D12Resource *buffer, *bufferupload;
} DX12BUFFER;

typedef struct {
  ID3D12Resource *buffer, *bufferupload;
  D3D12_VERTEX_BUFFER_VIEW view;
} DX12VERTEXBUFFER;

typedef struct {
  ID3D12Resource *buffer, *bufferupload;
  D3D12_INDEX_BUFFER_VIEW view;
} DX12INDEXBUFFER;