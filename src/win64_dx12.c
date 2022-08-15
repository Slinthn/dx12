void DXWaitForFence(DX12STATE *state) {
  // Get new fence value 
  UQWORD nextfence = state->fence->lpVtbl->GetCompletedValue(state->fence) + 1;

  // Create fence event
  HANDLE fenceevent = CreateEventA(0, 0, 0, 0);

  // Change the fence value on the GPU (but it only changes when all lists have finished executing)
  state->queue->lpVtbl->Signal(state->queue, state->fence, nextfence);

  // Check if the GPU has already finshed
  if (state->fence->lpVtbl->GetCompletedValue(state->fence) < nextfence) {
    // Tell the GPU to invoke this event once the fence value is reached (which happens when the lists are finished)
    state->fence->lpVtbl->SetEventOnCompletion(state->fence, nextfence, fenceevent);

    // Wait
    WaitForSingleObject(fenceevent, INFINITE);
  }

  // Delete the event
  CloseHandle(fenceevent);
}

// The GetCPUDescriptorHandleForHeapStart and GetGPUDescriptorHandleForHeapStart function signatures are INCORRECT on the documentation AND in <d3d12.h>.
// This appears to only be an issue when calling the C interfaces (through lpVtbl), but not when using the C++ interfaces.
// Hence, I created the DXGetCPUDescriptorHandleForHeapStart and DXGetGPUDescriptorHandleForHeapStart functions.
D3D12_CPU_DESCRIPTOR_HANDLE DXGetCPUDescriptorHandleForHeapStart(ID3D12DescriptorHeap *heap) {
#pragma warning(push)
#pragma warning(disable : 4020)
    D3D12_CPU_DESCRIPTOR_HANDLE cpudescriptor;
    heap->lpVtbl->GetCPUDescriptorHandleForHeapStart(heap, &cpudescriptor);
#pragma warning(pop)
    return cpudescriptor;
}

D3D12_GPU_DESCRIPTOR_HANDLE DXGetGPUDescriptorHandleForHeapStart(ID3D12DescriptorHeap *heap) {
#pragma warning(push)
#pragma warning(disable : 4020)
    D3D12_GPU_DESCRIPTOR_HANDLE gpudescriptor;
    heap->lpVtbl->GetGPUDescriptorHandleForHeapStart(heap, &gpudescriptor);
#pragma warning(pop)
    return gpudescriptor;
}

DX12STATE DXInit(HWND window) {
  DX12STATE state = {0};

  // Setup debug layer
  UDWORD flags = 0;
#ifdef _DEBUG
  flags |= DXGI_CREATE_FACTORY_DEBUG;

  ID3D12Debug *debug;
  D3D12GetDebugInterface(&IID_ID3D12Debug, &debug);
  debug->lpVtbl->EnableDebugLayer(debug);
#endif

  // Create DXGI factory
  IDXGIFactory2 *factory;
  CreateDXGIFactory2(flags, &IID_IDXGIFactory, &factory);

  // Query GPUs and create a device
  IDXGIAdapter *adapter;
  for (UDWORD i = 0; factory->lpVtbl->EnumAdapters(factory, i, &adapter) != DXGI_ERROR_NOT_FOUND; i++) {
    if (D3D12CreateDevice((IUnknown *)adapter, D3D_FEATURE_LEVEL_12_0, &IID_ID3D12Device, &state.device) == S_OK) {
      break;
    }
  }

  if (!state.device)
    ExitProcess(5); // TODO exit code

  // Setup debugg logging
#ifdef _DEBUG
  ID3D12InfoQueue *infoqueue;
  state.device->lpVtbl->QueryInterface(state.device, &IID_ID3D12InfoQueue, &infoqueue);
  infoqueue->lpVtbl->SetBreakOnSeverity(infoqueue, D3D12_MESSAGE_SEVERITY_CORRUPTION, 1);
  infoqueue->lpVtbl->SetBreakOnSeverity(infoqueue, D3D12_MESSAGE_SEVERITY_WARNING, 1);
  infoqueue->lpVtbl->SetBreakOnSeverity(infoqueue, D3D12_MESSAGE_SEVERITY_ERROR, 1);
#endif
  
  // Setup command queue
  D3D12_COMMAND_QUEUE_DESC commandqueuedesc = {0};
  commandqueuedesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  commandqueuedesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH;

  state.device->lpVtbl->CreateCommandQueue(state.device, &commandqueuedesc, &IID_ID3D12CommandQueue, &state.queue);

  // Create a command allocator
  state.device->lpVtbl->CreateCommandAllocator(state.device, 0, &IID_ID3D12CommandAllocator, &state.allocator);

  // Create a command list using the command allocator
  state.device->lpVtbl->CreateCommandList(state.device, 0, D3D12_COMMAND_LIST_TYPE_DIRECT, state.allocator, 0, &IID_ID3D12CommandList, &state.list);
  state.list->lpVtbl->Close(state.list);

  // Create fence for command queue
  state.device->lpVtbl->CreateFence(state.device, 0, 0, &IID_ID3D12Fence, &state.fence);

  // Create swap chain
  DXGI_SWAP_CHAIN_DESC1 swapchaindesc = {0};
  swapchaindesc.Width = WINDOW_WIDTH;
  swapchaindesc.Height = WINDOW_HEIGHT;
  swapchaindesc.BufferCount = 2;
  swapchaindesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapchaindesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapchaindesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
  swapchaindesc.SampleDesc.Count = 1;

  factory->lpVtbl->CreateSwapChainForHwnd(factory, (IUnknown *)state.queue, window, &swapchaindesc, 0, 0, (IDXGISwapChain1 **)&state.swapchain);

  // Create renter target view descriptor descriptorheap
  D3D12_DESCRIPTOR_HEAP_DESC descriptorheapdesc = {0};
  descriptorheapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  descriptorheapdesc.NumDescriptors = 2;

  state.device->lpVtbl->CreateDescriptorHeap(state.device, &descriptorheapdesc, &IID_ID3D12DescriptorHeap, &state.rendertargetviewdescriptorheap);

  // Retrieve render target resources (2)
  D3D12_CPU_DESCRIPTOR_HANDLE cpudescriptor = DXGetCPUDescriptorHandleForHeapStart(state.rendertargetviewdescriptorheap);
  UDWORD rtvDescriptorSize = state.device->lpVtbl->GetDescriptorHandleIncrementSize(state.device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  for (UDWORD i = 0; i < 2; i++) {
    state.swapchain->lpVtbl->GetBuffer(state.swapchain, i, &IID_ID3D12Resource, &state.rendertargets[i]);
    state.device->lpVtbl->CreateRenderTargetView(state.device, state.rendertargets[i], 0, cpudescriptor);
    cpudescriptor.ptr += rtvDescriptorSize;
  }

  // Create depth stencil descriptor descriptorheap
  descriptorheapdesc = (D3D12_DESCRIPTOR_HEAP_DESC){0};
  descriptorheapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
  descriptorheapdesc.NumDescriptors = 1;

  state.device->lpVtbl->CreateDescriptorHeap(state.device, &descriptorheapdesc, &IID_ID3D12DescriptorHeap, &state.depthstencilviewheap);

  // Create buffer for the depth stencil
  D3D12_HEAP_PROPERTIES heapproperties = {0};
  heapproperties.Type = D3D12_HEAP_TYPE_DEFAULT;
  heapproperties.CreationNodeMask = 1;
  heapproperties.VisibleNodeMask = 1;

  D3D12_RESOURCE_DESC resourcedesc = {0};
  resourcedesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  resourcedesc.Width = WINDOW_WIDTH;
  resourcedesc.Height = WINDOW_HEIGHT;
  resourcedesc.DepthOrArraySize = 1;
  resourcedesc.MipLevels = 0; // TODO debug
  resourcedesc.SampleDesc.Count = 1;
  resourcedesc.Format = DXGI_FORMAT_D32_FLOAT;
  resourcedesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

  D3D12_CLEAR_VALUE clearvalue = {0};
  clearvalue.Format = DXGI_FORMAT_D32_FLOAT;
  clearvalue.DepthStencil.Depth = 1.0f;

  ID3D12Resource *depthstencilresource;
  state.device->lpVtbl->CreateCommittedResource(state.device, &heapproperties, 0, &resourcedesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearvalue, &IID_ID3D12Resource, &depthstencilresource); // TODO make placed resource?

  // Retrieve depth stencil view
  D3D12_DEPTH_STENCIL_VIEW_DESC depthstencilviewdesc = {0};
  depthstencilviewdesc.Format = DXGI_FORMAT_D32_FLOAT;
  depthstencilviewdesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

  state.device->lpVtbl->CreateDepthStencilView(state.device, depthstencilresource, &depthstencilviewdesc, DXGetCPUDescriptorHandleForHeapStart(state.depthstencilviewheap));

  // Return
  return state;
}

DXSHADER DXCreateShader(DX12STATE *state, void *vcode, UDWORD vsize, void *pcode, UDWORD psize, D3D12_INPUT_ELEMENT_DESC ieds[], UDWORD iedcount) {
  DXSHADER shader = {0};

  // Create root signature from vertex shader
  state->device->lpVtbl->CreateRootSignature(state->device, 0, vcode, vsize, &IID_ID3D12RootSignature, &shader.rootsignature);

  // Create graphics pipeline state object
  D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsd = {0};
  gpsd.pRootSignature = shader.rootsignature;
  gpsd.VS.pShaderBytecode = vcode;
  gpsd.VS.BytecodeLength = vsize;
  gpsd.PS.pShaderBytecode = pcode;
  gpsd.PS.BytecodeLength = psize;
  gpsd.SampleMask = UINT_MAX;
  gpsd.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
  gpsd.RasterizerState.CullMode = D3D12_CULL_MODE_NONE ;//D3D12_CULL_MODE_BACK; // TODO this
  gpsd.RasterizerState.FrontCounterClockwise = 0; // TODO this maybe
  gpsd.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
  gpsd.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
  gpsd.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
  gpsd.RasterizerState.DepthClipEnable = 0; // TODO debyug
  gpsd.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
  gpsd.DepthStencilState.DepthEnable = 1;
  gpsd.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
  gpsd.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
  gpsd.DepthStencilState.FrontFace = (D3D12_DEPTH_STENCILOP_DESC){D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS};
  gpsd.DepthStencilState.BackFace = (D3D12_DEPTH_STENCILOP_DESC){D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS};
  gpsd.InputLayout.pInputElementDescs = ieds;
  gpsd.InputLayout.NumElements = iedcount;
  gpsd.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  gpsd.NumRenderTargets = 1;
  gpsd.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
  gpsd.DSVFormat = DXGI_FORMAT_D32_FLOAT;
  gpsd.SampleDesc.Count = 1;

  for (UDWORD i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++) {
    gpsd.BlendState.RenderTarget[i] = (D3D12_RENDER_TARGET_BLEND_DESC){0, 0, D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD, D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD, D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL};
  }

  state->device->lpVtbl->CreateGraphicsPipelineState(state->device, &gpsd, &IID_ID3D12PipelineState, &shader.pipeline);

  // Return
  return shader;
}
ID3D12Heap *DXCreateHeap(DX12STATE *state, UQWORD sizeinbytes, D3D12_HEAP_TYPE type) {
  // Create heap and align to 64KB
  D3D12_HEAP_DESC heapdesc = {0};
  heapdesc.SizeInBytes = sizeinbytes - (sizeinbytes % (64 * 1024)) + (64 * 1024);
  heapdesc.Properties.Type = type;

  ID3D12Heap *heap;
  state->device->lpVtbl->CreateHeap(state->device, &heapdesc, &IID_ID3D12Heap, &heap);

  // Return
  return heap;
}

ID3D12Resource *DXCreateBufferResource(DX12STATE *state, UQWORD sizeinbytes, ID3D12Heap *heap, D3D12_RESOURCE_STATES resourcestate, UQWORD offset) {
  // Reserve memory to a resource on a heap
  D3D12_RESOURCE_DESC resourcedesc = {0};
  resourcedesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourcedesc.Width = sizeinbytes;
  resourcedesc.Height = 1;
  resourcedesc.DepthOrArraySize = 1;
  resourcedesc.MipLevels = 1;
  resourcedesc.SampleDesc.Count = 1;
  resourcedesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  ID3D12Resource *resource;
  state->device->lpVtbl->CreatePlacedResource(state->device, heap, offset, &resourcedesc, resourcestate, 0, &IID_ID3D12Resource, &resource);

  // Return
  return resource;
}

DX12BUFFER DXCreateAndUploadBuffer(DX12STATE *state, ID3D12Heap *heap, ID3D12Heap *uploadheap, void *data, UQWORD sizeinbytes, UQWORD offsetinbytes, D3D12_RESOURCE_STATES prevstate) {
  DX12BUFFER buffer = {0};

  // Create normal and upload resource
  buffer.buffer = DXCreateBufferResource(state, sizeinbytes, heap, prevstate, offsetinbytes);
  buffer.bufferupload = DXCreateBufferResource(state, sizeinbytes, uploadheap, D3D12_RESOURCE_STATE_GENERIC_READ, offsetinbytes);

  // Map and upload data
  D3D12_RANGE range = {0};

  void *resourcedata;
  buffer.bufferupload->lpVtbl->Map(buffer.bufferupload, 0, &range, &resourcedata);

  CopyMemory(resourcedata, data, sizeinbytes);

  // Change states
  D3D12_RESOURCE_BARRIER rb = {0};
  rb.Transition.pResource = buffer.buffer;
  rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  rb.Transition.StateBefore = prevstate;
  rb.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
  state->list->lpVtbl->ResourceBarrier(state->list, 1, &rb);

  // Copy the upload buffer to the actual buffer
  state->list->lpVtbl->CopyResource(state->list, buffer.buffer, buffer.bufferupload);
  buffer.bufferupload->lpVtbl->Unmap(buffer.bufferupload, 0, &range);

  // Change state back
  rb.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
  rb.Transition.StateAfter = prevstate;
  state->list->lpVtbl->ResourceBarrier(state->list, 1, &rb);

  // Return
  return buffer;
}

DX12VERTEXBUFFER DXCreateAndUploadVertexBuffer(DX12STATE *state, ID3D12Heap *heap, ID3D12Heap *uploadheap, void *data, UQWORD sizeinbytes, UQWORD offsetinbytes) {
  DX12VERTEXBUFFER vertexbuffer = {0};
  
  // Create upload buffer and normal buffer
  DX12BUFFER buffer = DXCreateAndUploadBuffer(state, heap, uploadheap, data, sizeinbytes, offsetinbytes, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

  vertexbuffer.buffer = buffer.buffer;
  vertexbuffer.bufferupload = buffer.bufferupload;

  // Setup vertex buffer view
  vertexbuffer.view.BufferLocation = buffer.buffer->lpVtbl->GetGPUVirtualAddress(buffer.buffer);
  vertexbuffer.view.StrideInBytes = sizeof(VERTEX);
  vertexbuffer.view.SizeInBytes = (UDWORD)sizeinbytes;

  // Return
  return vertexbuffer;
}

DX12INDEXBUFFER DXCreateAndUploadIndexBuffer(DX12STATE *state, ID3D12Heap *heap, ID3D12Heap *uploadheap, void *data, UQWORD sizeinbytes, UQWORD offsetinbytes, DXGI_FORMAT format) {
  DX12INDEXBUFFER indexbuffer = {0};

  // Create upload buffer and normal buffer
  DX12BUFFER buffer = DXCreateAndUploadBuffer(state, heap, uploadheap, data, sizeinbytes, offsetinbytes, D3D12_RESOURCE_STATE_INDEX_BUFFER);

  indexbuffer.buffer = buffer.buffer;
  indexbuffer.bufferupload = buffer.bufferupload;

  // Setup index buffer view
  indexbuffer.view.BufferLocation = buffer.buffer->lpVtbl->GetGPUVirtualAddress(buffer.buffer);
  indexbuffer.view.Format = format;
  indexbuffer.view.SizeInBytes = (UDWORD)sizeinbytes;

  // Return
  return indexbuffer;
}

DX12TEXTURE DXCreateAndUploadTexture(DX12STATE *state, UDWORD width, UDWORD height, ID3D12Heap *heap, ID3D12Heap *uploadheap, void *data, UQWORD offsetinbytes) {
  DX12TEXTURE texture = {0};
  
  // Reserve memory for the resource on the heap
  D3D12_RESOURCE_DESC resourcedesc = {0};
  resourcedesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  resourcedesc.Width = width;
  resourcedesc.Height = height;
  resourcedesc.DepthOrArraySize = 1;
  resourcedesc.MipLevels = 1;
  resourcedesc.SampleDesc.Count = 1;
  resourcedesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

  state->device->lpVtbl->CreatePlacedResource(state->device, heap, offsetinbytes, &resourcedesc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0, &IID_ID3D12Resource, &texture.texture);

  // Reserve memory for the resource on the upload heap using same variable
  resourcedesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourcedesc.Width = AlignUp(width, 256) * height * 4;
  resourcedesc.Height = 1;
  resourcedesc.Format = DXGI_FORMAT_UNKNOWN;
  resourcedesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  state->device->lpVtbl->CreatePlacedResource(state->device, uploadheap, offsetinbytes, &resourcedesc, D3D12_RESOURCE_STATE_GENERIC_READ, 0, &IID_ID3D12Resource, &texture.uploadtexture);

  // Get pointer to upload resource
  void *ptr;
  D3D12_RANGE range = {0};
  texture.uploadtexture->lpVtbl->Map(texture.uploadtexture, 0, &range, &ptr);

  // Fill out structures to copy the upload resource to the actual texture resource (used later)
  D3D12_TEXTURE_COPY_LOCATION dst = {0};
  dst.pResource = texture.texture;

  D3D12_TEXTURE_COPY_LOCATION src = {0};
  src.pResource = texture.uploadtexture;
  src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
  src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  src.PlacedFootprint.Footprint.Width = width;
  src.PlacedFootprint.Footprint.Height = height;
  src.PlacedFootprint.Footprint.Depth = 1;
  src.PlacedFootprint.Footprint.RowPitch = AlignUp(width * 4, 256);
  src.PlacedFootprint.Offset = AlignUp((UQWORD)ptr, 512) - (UQWORD)ptr;

  // Copy memory from RAM to the upload resource
  for (UDWORD i = 0; i < height; i++) {
    void *srcptr = (void *)((UQWORD)data + (UQWORD)(width * i * 4));
    void *destptr = (void *)((UQWORD)ptr + src.PlacedFootprint.Offset + (UQWORD)(src.PlacedFootprint.Footprint.RowPitch * i));
    CopyMemory(destptr, srcptr, width * 4);
  }

  // Unmap upload resource
  texture.uploadtexture->lpVtbl->Unmap(texture.uploadtexture, 0, 0);

  // Change resource state and copy memory from upload resource to texture using structure filled out above
  D3D12_RESOURCE_BARRIER rb = {0};
  rb.Transition.pResource = texture.texture;
  rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  rb.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
  rb.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
  state->list->lpVtbl->ResourceBarrier(state->list, 1, &rb);

  state->list->lpVtbl->CopyTextureRegion(state->list, &dst, 0, 0, 0, &src, 0);

  // Change state back to shader resource
  rb.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
  rb.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
  state->list->lpVtbl->ResourceBarrier(state->list, 1, &rb);

  // Return
  return texture;
}