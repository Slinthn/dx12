struct dx12_descriptor_handle {
  D3D12_CPU_DESCRIPTOR_HANDLE cpuhandle;
  D3D12_GPU_DESCRIPTOR_HANDLE gpuhandle;
};

struct dx12_descriptor_heap {
  uint32_t count;
  uint32_t usedcount;
  D3D12_DESCRIPTOR_HEAP_TYPE type;
  uint8_t unused0[4];
  ID3D12DescriptorHeap *heap;
};

struct dx12_shader {
  ID3D12RootSignature *rootsignature;
  ID3D12PipelineState *pipeline;
};

struct dx12_texture {
  ID3D12Resource *texture, *uploadtexture;
};

struct dx12_sampler {
  ID3D12DescriptorHeap *heap;
};

struct dx12_state {
  ID3D12Device *device;
  ID3D12CommandQueue *queue;
  IDXGISwapChain3 *swapchain;
  ID3D12DescriptorHeap *rendertargetviewdescriptorheap;
  ID3D12Resource *rendertargets[2];
  ID3D12CommandAllocator *allocator;
  ID3D12GraphicsCommandList *list;
  ID3D12Fence *fence;
  ID3D12DescriptorHeap *depthstencilviewdescriptorheap;
};

struct dx12_buffer {
  ID3D12Resource *buffer, *bufferupload;
};

struct dx12_vertexbuffer {
  ID3D12Resource *buffer, *bufferupload;
  D3D12_VERTEX_BUFFER_VIEW view;
};

struct dx12_indexbuffer {
  ID3D12Resource *buffer, *bufferupload;
  D3D12_INDEX_BUFFER_VIEW view;
};

struct dx12_shadow {
  ID3D12Resource *depthresource;
  ID3D12DescriptorHeap *descriptorheap;
  struct dx12_descriptor_handle texturehandle;
};