void dx12_wait_for_fence(dx12_state *state) {
  // Get new fence value 
  u64 nextfence = state->fence->lpVtbl->GetCompletedValue(state->fence) + 1;

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
// Hence, I created the dx12_get_cpu_descriptor_handle_for_heap_start and dx12_get_gpu_descriptor_handle_for_heap_start functions.
D3D12_CPU_DESCRIPTOR_HANDLE dx12_get_cpu_descriptor_handle_for_heap_start(ID3D12DescriptorHeap *heap) {
#pragma warning(push)
#pragma warning(disable : 4020)
    D3D12_CPU_DESCRIPTOR_HANDLE cpudescriptor;
    heap->lpVtbl->GetCPUDescriptorHandleForHeapStart(heap, &cpudescriptor);
#pragma warning(pop)
    return cpudescriptor;
}

D3D12_GPU_DESCRIPTOR_HANDLE dx12_get_gpu_descriptor_handle_for_heap_start(ID3D12DescriptorHeap *heap) {
#pragma warning(push)
#pragma warning(disable : 4020)
    D3D12_GPU_DESCRIPTOR_HANDLE gpudescriptor;
    heap->lpVtbl->GetGPUDescriptorHandleForHeapStart(heap, &gpudescriptor);
#pragma warning(pop)
    return gpudescriptor;
}

dx12_state dx12_init(HWND window, BOOL debug) {
  dx12_state state = {0};

  // Setup debug layer
  u32 flags = 0;
  if (debug) {
    flags |= DXGI_CREATE_FACTORY_DEBUG;

    ID3D12Debug *debuginterface;
    D3D12GetDebugInterface(&IID_ID3D12Debug, &debuginterface);
    debuginterface->lpVtbl->EnableDebugLayer(debuginterface);
  }

  // Create DXGI factory
  IDXGIFactory2 *factory;
  CreateDXGIFactory2(flags, &IID_IDXGIFactory, &factory);

  // Query GPUs and create a device
  IDXGIAdapter *adapter;
  for (u32 i = 0; factory->lpVtbl->EnumAdapters(factory, i, &adapter) != DXGI_ERROR_NOT_FOUND; i++) {
    if (D3D12CreateDevice((IUnknown *)adapter, D3D_FEATURE_LEVEL_12_0, &IID_ID3D12Device, &state.device) == S_OK) {
      break;
    }
  }

  if (!state.device) {
    FATAL_ERROR("Failed to initialise DirectX 12.");
  }

  // Setup debug logging
  if (debug) {
    ID3D12InfoQueue *infoqueue;
    state.device->lpVtbl->QueryInterface(state.device, &IID_ID3D12InfoQueue, &infoqueue);
    infoqueue->lpVtbl->SetBreakOnSeverity(infoqueue, D3D12_MESSAGE_SEVERITY_CORRUPTION, 1);
    infoqueue->lpVtbl->SetBreakOnSeverity(infoqueue, D3D12_MESSAGE_SEVERITY_WARNING, 1);
    infoqueue->lpVtbl->SetBreakOnSeverity(infoqueue, D3D12_MESSAGE_SEVERITY_ERROR, 1);
  }
  
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

  // Create renter target view descriptor heap
  D3D12_DESCRIPTOR_HEAP_DESC descriptorheapdesc = {0};
  descriptorheapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  descriptorheapdesc.NumDescriptors = 2;

  state.device->lpVtbl->CreateDescriptorHeap(state.device, &descriptorheapdesc, &IID_ID3D12DescriptorHeap, &state.rendertargetviewdescriptorheap);

  // Retrieve render target resources (2)
  D3D12_CPU_DESCRIPTOR_HANDLE cpudescriptor = dx12_get_cpu_descriptor_handle_for_heap_start(state.rendertargetviewdescriptorheap);
  u32 rtvDescriptorSize = state.device->lpVtbl->GetDescriptorHandleIncrementSize(state.device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  for (u32 i = 0; i < 2; i++) {
    state.swapchain->lpVtbl->GetBuffer(state.swapchain, i, &IID_ID3D12Resource, &state.rendertargets[i]);
    state.device->lpVtbl->CreateRenderTargetView(state.device, state.rendertargets[i], 0, cpudescriptor);
    cpudescriptor.ptr += rtvDescriptorSize;
  }

  // Create depth stencil descriptor heap
  descriptorheapdesc = (D3D12_DESCRIPTOR_HEAP_DESC){0};
  descriptorheapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
  descriptorheapdesc.NumDescriptors = 1;

  state.device->lpVtbl->CreateDescriptorHeap(state.device, &descriptorheapdesc, &IID_ID3D12DescriptorHeap, &state.depthstencilviewdescriptorheap);

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
  resourcedesc.MipLevels = 0;
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

  state.device->lpVtbl->CreateDepthStencilView(state.device, depthstencilresource, &depthstencilviewdesc, dx12_get_cpu_descriptor_handle_for_heap_start(state.depthstencilviewdescriptorheap));

  // Return
  return state;
}

dx12_shader dx12_create_shader(dx12_state *state, void *vcode, u32 vsize, void *pcode, u32 psize, D3D12_INPUT_LAYOUT_DESC inputlayoutdesc) {
  dx12_shader shader = {0};

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
  gpsd.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // TODO D3D12_CULL_MODE_BACK 
  gpsd.RasterizerState.FrontCounterClockwise = 0;
  gpsd.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
  gpsd.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
  gpsd.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
  gpsd.RasterizerState.DepthClipEnable = 0;
  gpsd.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
  gpsd.DepthStencilState.DepthEnable = 1;
  gpsd.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
  gpsd.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
  gpsd.DepthStencilState.FrontFace = (D3D12_DEPTH_STENCILOP_DESC){D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS};
  gpsd.DepthStencilState.BackFace = (D3D12_DEPTH_STENCILOP_DESC){D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS};
  gpsd.InputLayout = inputlayoutdesc;
  gpsd.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  gpsd.NumRenderTargets = 1;
  gpsd.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
  gpsd.DSVFormat = DXGI_FORMAT_D32_FLOAT;
  gpsd.SampleDesc.Count = 1;

  for (u32 i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++) {
    gpsd.BlendState.RenderTarget[i] = (D3D12_RENDER_TARGET_BLEND_DESC){0, 0, D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD, D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD, D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL};
  }

  state->device->lpVtbl->CreateGraphicsPipelineState(state->device, &gpsd, &IID_ID3D12PipelineState, &shader.pipeline);

  // Return
  return shader;
}
ID3D12Heap *dx12_create_heap(dx12_state *state, u64 sizeinbytes, D3D12_HEAP_TYPE type) {
  // Create heap and align to 64KB
  D3D12_HEAP_DESC heapdesc = {0};
  heapdesc.SizeInBytes = ALIGN_UP(sizeinbytes, 1024 * 64);
  heapdesc.Properties.Type = type;

  ID3D12Heap *heap;
  state->device->lpVtbl->CreateHeap(state->device, &heapdesc, &IID_ID3D12Heap, &heap);

  // Return
  return heap;
}

ID3D12Resource *dx12_create_buffer_resource(dx12_state *state, u64 sizeinbytes, ID3D12Heap *heap, D3D12_RESOURCE_STATES resourcestate, u64 offset) {
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

dx12_buffer dx12_create_and_upload_buffer(dx12_state *state, ID3D12Heap *heap, ID3D12Heap *uploadheap, void *data, u64 sizeinbytes, u64 offsetinbytes, D3D12_RESOURCE_STATES prevstate) {
  dx12_buffer buffer = {0};

  // Create normal and upload resource
  buffer.buffer = dx12_create_buffer_resource(state, sizeinbytes, heap, prevstate, offsetinbytes);
  buffer.bufferupload = dx12_create_buffer_resource(state, sizeinbytes, uploadheap, D3D12_RESOURCE_STATE_GENERIC_READ, offsetinbytes);

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

dx12_vertexbuffer dx12_create_and_upload_vertex_buffer(dx12_state *state, ID3D12Heap *heap, ID3D12Heap *uploadheap, void *data, u64 sizeinbytes, u64 offsetinbytes, u32 strideinbytes) {
  dx12_vertexbuffer vertexbuffer = {0};
  
  // Create upload buffer and normal buffer
  dx12_buffer buffer = dx12_create_and_upload_buffer(state, heap, uploadheap, data, sizeinbytes, offsetinbytes, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

  vertexbuffer.buffer = buffer.buffer;
  vertexbuffer.bufferupload = buffer.bufferupload;

  // Setup vertex buffer view
  vertexbuffer.view.BufferLocation = buffer.buffer->lpVtbl->GetGPUVirtualAddress(buffer.buffer);
  vertexbuffer.view.StrideInBytes = strideinbytes;
  vertexbuffer.view.SizeInBytes = (u32)sizeinbytes;

  // Return
  return vertexbuffer;
}

dx12_indexbuffer dx12_create_and_upload_index_buffer(dx12_state *state, ID3D12Heap *heap, ID3D12Heap *uploadheap, void *data, u64 sizeinbytes, u64 offsetinbytes, DXGI_FORMAT format) {
  dx12_indexbuffer indexbuffer = {0};

  // Create upload buffer and normal buffer
  dx12_buffer buffer = dx12_create_and_upload_buffer(state, heap, uploadheap, data, sizeinbytes, offsetinbytes, D3D12_RESOURCE_STATE_INDEX_BUFFER);

  indexbuffer.buffer = buffer.buffer;
  indexbuffer.bufferupload = buffer.bufferupload;

  // Setup index buffer view
  indexbuffer.view.BufferLocation = buffer.buffer->lpVtbl->GetGPUVirtualAddress(buffer.buffer);
  indexbuffer.view.Format = format;
  indexbuffer.view.SizeInBytes = (u32)sizeinbytes;

  // Return
  return indexbuffer;
}

dx12_texture dx12_create_and_upload_texture(dx12_state *state, u32 width, u32 height, ID3D12Heap *heap, ID3D12Heap *uploadheap, void *data, u64 offsetinbytes) {
  dx12_texture texture = {0};
  
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
  resourcedesc.Width = TRUE_IMAGE_SIZE_IN_BYTES(width, height);
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
  src.PlacedFootprint.Footprint.RowPitch = ALIGN_UP(width * 4, 256);
  src.PlacedFootprint.Offset = ALIGN_UP((u64)ptr, 512) - (u64)ptr;

  // Copy memory from RAM to the upload resource
  for (u32 i = 0; i < height; i++) {
    void *srcptr = (void *)((u64)data + (u64)(width * i * 4));
    void *destptr = (void *)((u64)ptr + src.PlacedFootprint.Offset + (u64)(src.PlacedFootprint.Footprint.RowPitch * i));
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

dx12_sampler dx12_create_sampler(dx12_state *state) {
  dx12_sampler sampler = {0};

  // Create descriptor heap
  D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {0};
  samplerHeapDesc.NumDescriptors = 1;
  samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
  samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

  state->device->lpVtbl->CreateDescriptorHeap(state->device, &samplerHeapDesc, &IID_ID3D12DescriptorHeap, &sampler.heap);

  // Create sampler
  D3D12_SAMPLER_DESC samplerdesc = {0};
  samplerdesc.Filter = D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
  samplerdesc.AddressU = samplerdesc.AddressV = samplerdesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  samplerdesc.MaxLOD = D3D12_FLOAT32_MAX;
  samplerdesc.MaxAnisotropy = 1;
  samplerdesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;

  state->device->lpVtbl->CreateSampler(state->device, &samplerdesc, dx12_get_cpu_descriptor_handle_for_heap_start(sampler.heap));

  // Return
  return sampler;
}

#include "descriptorheap.c"

dx12_shadow dx12_create_shadow_buffer(dx12_state *state, dx12_descriptor_heap *texturedescriptorheap) {
  dx12_shadow shadow = {0};

  // Create descriptor heap for the shader with a single entry for the depth stencil view
  D3D12_DESCRIPTOR_HEAP_DESC descriptorheapdesc = {0};
  descriptorheapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
  descriptorheapdesc.NumDescriptors = 1;

  state->device->lpVtbl->CreateDescriptorHeap(state->device, &descriptorheapdesc, &IID_ID3D12DescriptorHeap, &shadow.descriptorheap);

  D3D12_HEAP_PROPERTIES heapproperties = {0};
  heapproperties.Type = D3D12_HEAP_TYPE_DEFAULT;
  heapproperties.CreationNodeMask = 1;
  heapproperties.VisibleNodeMask = 1;

  D3D12_RESOURCE_DESC resourcedesc = {0};
  resourcedesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  resourcedesc.Width = SHADOW_WIDTH;
  resourcedesc.Height = SHADOW_HEIGHT;
  resourcedesc.DepthOrArraySize = 1;
  resourcedesc.MipLevels = 0;
  resourcedesc.SampleDesc.Count = 1;
  resourcedesc.Format = DXGI_FORMAT_R32_TYPELESS;
  resourcedesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

  D3D12_CLEAR_VALUE clearvalue = {0};
  clearvalue.Format = DXGI_FORMAT_D32_FLOAT;
  clearvalue.DepthStencil.Depth = 1.0f;

  state->device->lpVtbl->CreateCommittedResource(state->device, &heapproperties, 0, &resourcedesc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearvalue, &IID_ID3D12Resource, &shadow.depthresource); // TODO make placed resource?

  // Retrieve depth stencil view
  D3D12_DEPTH_STENCIL_VIEW_DESC depthstencilviewdesc = {0};
  depthstencilviewdesc.Format = DXGI_FORMAT_D32_FLOAT;
  depthstencilviewdesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

  state->device->lpVtbl->CreateDepthStencilView(state->device, shadow.depthresource, &depthstencilviewdesc, dx12_get_cpu_descriptor_handle_for_heap_start(shadow.descriptorheap));

  D3D12_SHADER_RESOURCE_VIEW_DESC shaderresourceviewdesc = {0};
  shaderresourceviewdesc.Format = DXGI_FORMAT_R32_FLOAT;
  shaderresourceviewdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  shaderresourceviewdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  shaderresourceviewdesc.Texture2D.MipLevels = 1;

  shadow.texturehandle = dx12_get_next_unused_handle(state, texturedescriptorheap);

  state->device->lpVtbl->CreateShaderResourceView(state->device, shadow.depthresource, &shaderresourceviewdesc, shadow.texturehandle.cpuhandle);

  // Return
  return shadow;
}

void dx12_update_buffer(dx12_state *state, ID3D12Resource *constantbuffer0, ID3D12Resource *constantbuffer0upload, void *data) {
  D3D12_RANGE range = {0};
  void *resourcedata;
  constantbuffer0upload->lpVtbl->Map(constantbuffer0upload, 0, &range, &resourcedata);
  CopyMemory(resourcedata, data, sizeof(constant_buffer0));
  constantbuffer0upload->lpVtbl->Unmap(constantbuffer0upload, 0, &range);

  // Update constant buffer using data from upload buffer
  D3D12_RESOURCE_BARRIER rb = {0};
  rb.Transition.pResource = constantbuffer0;
  rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  rb.Transition.StateBefore = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
  rb.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
  state->list->lpVtbl->ResourceBarrier(state->list, 1, &rb);

  state->list->lpVtbl->CopyResource(state->list, constantbuffer0, constantbuffer0upload);

  // Set contant buffer back to a readable state
  rb.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
  rb.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
  state->list->lpVtbl->ResourceBarrier(state->list, 1, &rb);
}