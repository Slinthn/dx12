struct dx12_descriptor_heap dx12_create_descriptor_heap(
  struct dx12_state *state,
  uint32_t count,
  D3D12_DESCRIPTOR_HEAP_TYPE type,
  D3D12_DESCRIPTOR_HEAP_FLAGS flags)
{
  struct dx12_descriptor_heap ret = {0};

  D3D12_DESCRIPTOR_HEAP_DESC descriptorheapdesc = {0};
  descriptorheapdesc.Type = type;
  descriptorheapdesc.NumDescriptors = count;
  descriptorheapdesc.Flags = flags;

  state->device->lpVtbl->CreateDescriptorHeap(state->device,
    &descriptorheapdesc, &IID_ID3D12DescriptorHeap, &ret.heap);

  ret.count = count;
  ret.type = type;

  return ret;
}

struct dx12_descriptor_handle dx12_get_next_unused_handle(
  struct dx12_state *state,
  struct dx12_descriptor_heap *heap)
{
  struct dx12_descriptor_handle ret = {0};

  uint32_t increment =
    state->device->lpVtbl->GetDescriptorHandleIncrementSize(state->device,
    heap->type);

  ret.cpuhandle = dx12_get_cpu_descriptor_handle_for_heap_start(heap->heap);
  ret.cpuhandle.ptr += increment * heap->usedcount;

  ret.gpuhandle = dx12_get_gpu_descriptor_handle_for_heap_start(heap->heap);
  ret.gpuhandle.ptr += increment * heap->usedcount;

  heap->usedcount++;

  return ret;
}