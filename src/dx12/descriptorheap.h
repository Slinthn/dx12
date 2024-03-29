typedef struct t_DX12DESCRIPTORHANDLE {
  D3D12_CPU_DESCRIPTOR_HANDLE cpuhandle;
  D3D12_GPU_DESCRIPTOR_HANDLE gpuhandle;
} DX12DESCRIPTORHANDLE;

typedef struct t_DX12DESCRIPTORHEAP {
  UDWORD count;
  UDWORD usedcount;
  D3D12_DESCRIPTOR_HEAP_TYPE type;
  UBYTE unused0[4];
  ID3D12DescriptorHeap *heap;
} DX12DESCRIPTORHEAP;