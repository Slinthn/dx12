DX12DESCRIPTORHEAP DXCreateDescriptorHeap(DX12STATE *state, U32 count, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags) {
  DX12DESCRIPTORHEAP ret = {0};

  D3D12_DESCRIPTOR_HEAP_DESC descriptorheapdesc = {0};
  descriptorheapdesc.Type = type;
  descriptorheapdesc.NumDescriptors = count;
  descriptorheapdesc.Flags = flags;

  state->device->lpVtbl->CreateDescriptorHeap(state->device, &descriptorheapdesc, &IID_ID3D12DescriptorHeap, &ret.heap);

  ret.count = count;
  ret.type = type;

  return ret;
}

DX12DESCRIPTORHANDLE DXGetNextUnusedHandle(DX12STATE *state, DX12DESCRIPTORHEAP *heap) {
  DX12DESCRIPTORHANDLE ret = {0};

  U32 increment = state->device->lpVtbl->GetDescriptorHandleIncrementSize(state->device, heap->type);

  ret.cpuhandle = DXGetCPUDescriptorHandleForHeapStart(heap->heap);
  ret.cpuhandle.ptr += increment * heap->usedcount;

  ret.gpuhandle = DXGetGPUDescriptorHandleForHeapStart(heap->heap);
  ret.gpuhandle.ptr += increment * heap->usedcount;

  heap->usedcount++;

  return ret;
}