#pragma pack(push, 1)
typedef struct {
  MATRIX perspective;
  MATRIX transform;
  MATRIX camera;
} CB0;
#pragma pack(pop)

typedef struct {
  void *cbptrs[10]; // TODO max 10 here
  ID3D12DescriptorHeap *cbvheap;
  ID3D12RootSignature *rootsignature;
  ID3D12PipelineState *pipeline;
} DXSHADER;

typedef struct {
  ID3D12Heap *heap, *uploadheap;
  ID3D12Resource *constantbuffer0, *constantbuffer0upload;
} HEAP0;

typedef struct {
  ID3D12Heap *heap, *uploadheap;
  ID3D12Resource *vertexbuffer, *vertexbufferupload;
  ID3D12Resource *indexbuffer, *indexbufferupload;
} HEAP1;

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
  DX12SAMPLER sampler;
  
  HEAP0 heap0;
  HEAP1 heap1;
  ID3D12DescriptorHeap *descriptorheap;
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

#pragma pack(push, 1)
typedef struct {
  float position[3];
  float texture[2];
  float normal[3];
} VERTEX;
#pragma pack(pop)
