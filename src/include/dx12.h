typedef struct t_dx12_descriptor_handle {
  D3D12_CPU_DESCRIPTOR_HANDLE cpuhandle;
  D3D12_GPU_DESCRIPTOR_HANDLE gpuhandle;
} dx12_descriptor_handle;

typedef struct t_dx12_descriptor_heap {
  u32 count;
  u32 usedcount;
  D3D12_DESCRIPTOR_HEAP_TYPE type;
  u8 unused0[4];
  ID3D12DescriptorHeap *heap;
} dx12_descriptor_heap;

typedef struct t_dx12_shader {
  ID3D12RootSignature *rootsignature;
  ID3D12PipelineState *pipeline;
} dx12_shader;

typedef struct t_dx12_texture {
  ID3D12Resource *texture, *uploadtexture;
} dx12_texture;

typedef struct t_dx12_sampler {
  ID3D12DescriptorHeap *heap;
} dx12_sampler;

typedef struct t_dx12_state {
  ID3D12Device *device;
  ID3D12CommandQueue *queue;
  IDXGISwapChain3 *swapchain;
  ID3D12DescriptorHeap *rendertargetviewdescriptorheap;
  ID3D12Resource *rendertargets[2];
  ID3D12CommandAllocator *allocator;
  ID3D12GraphicsCommandList *list;
  ID3D12Fence *fence;
  ID3D12DescriptorHeap *depthstencilviewdescriptorheap;
} dx12_state;

typedef struct t_dx12_buffer {
  ID3D12Resource *buffer, *bufferupload;
} dx12_buffer;

typedef struct t_dx12_vertexbuffer {
  ID3D12Resource *buffer, *bufferupload;
  D3D12_VERTEX_BUFFER_VIEW view;
} dx12_vertexbuffer;

typedef struct t_dx12_indexbuffer {
  ID3D12Resource *buffer, *bufferupload;
  D3D12_INDEX_BUFFER_VIEW view;
} dx12_indexbuffer;

typedef struct t_dx12_shadow {
  ID3D12Resource *depthresource;
  ID3D12DescriptorHeap *descriptorheap;
  dx12_descriptor_handle texturehandle;
} dx12_shadow;