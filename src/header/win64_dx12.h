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
  ID3D12Debug *debug;
  IDXGIFactory4 *factory;
  IDXGIAdapter *adapter;
  ID3D12Device *device;
  ID3D12InfoQueue *infoqueue;
  ID3D12CommandQueue *queue;
  IDXGISwapChain3 *swapchain;
  ID3D12DescriptorHeap *rtvheap;
  ID3D12Resource *rendertargets[2];
  ID3D12CommandAllocator *allocator;
  ID3D12GraphicsCommandList *list;
  ID3D12Fence *fence;
  HANDLE fenceevent;
  UQWORD fencevalue;
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
  float normal[3];
  float texture[2];
} VERTEX;
#pragma pack(pop)