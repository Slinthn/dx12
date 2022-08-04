void DXWaitForFence(DX12STATE *state) {
  unsigned long long fence = state->fencevalue++;
  state->queue->lpVtbl->Signal(state->queue, state->fence, fence);
  if (state->fence->lpVtbl->GetCompletedValue(state->fence) < fence) {
    state->fence->lpVtbl->SetEventOnCompletion(state->fence, fence, state->fenceevent);
    WaitForSingleObject(state->fenceevent, INFINITE);
  }
}

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

void DXPrepareFrame(DX12STATE *state) {
  unsigned int frame = state->swapchain->lpVtbl->GetCurrentBackBufferIndex(state->swapchain);

  // TODO i moved this code into the app.c file. should it be here instead? separate function?
#if 0 
  state->allocator->lpVtbl->Reset(state->allocator);
  state->list->lpVtbl->Reset(state->list, state->allocator, state->pipeline);
  state->list->lpVtbl->SetGraphicsRootSignature(state->list, state->rootsignature);
  state->list->lpVtbl->SetDescriptorHeaps(state->list, 1, &state->cbvheap);
  state->list->lpVtbl->SetGraphicsRootDescriptorTable(state->list, 0, DXGetGPUDescriptorHandleForHeapStart(state->cbvheap));
#endif

  D3D12_VIEWPORT viewport = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
  state->list->lpVtbl->RSSetViewports(state->list, 1, &viewport);

  D3D12_RECT scissor = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
  state->list->lpVtbl->RSSetScissorRects(state->list, 1, &scissor);

  D3D12_RESOURCE_BARRIER rb = {0};
  rb.Transition.pResource = state->rendertargets[frame];
  rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  rb.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
  state->list->lpVtbl->ResourceBarrier(state->list, 1, &rb);

  D3D12_CPU_DESCRIPTOR_HANDLE cpudescriptor = DXGetCPUDescriptorHandleForHeapStart(state->rtvheap);
  cpudescriptor.ptr += frame * state->device->lpVtbl->GetDescriptorHandleIncrementSize(state->device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  D3D12_CPU_DESCRIPTOR_HANDLE dsvdescriptor = DXGetCPUDescriptorHandleForHeapStart(state->dsvheap); 
  state->list->lpVtbl->OMSetRenderTargets(state->list, 1, &cpudescriptor, 0, &dsvdescriptor);

  state->list->lpVtbl->ClearRenderTargetView(state->list, cpudescriptor, (float[4]) {1.0f, 1.0f, 1.0f, 1.0f}, 0, 0);
  state->list->lpVtbl->ClearDepthStencilView(state->list, DXGetCPUDescriptorHandleForHeapStart(state->dsvheap), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, 0);
}

void DXFlushFrame(DX12STATE *state) {
  unsigned int frame = state->swapchain->lpVtbl->GetCurrentBackBufferIndex(state->swapchain);

  D3D12_RESOURCE_BARRIER rb = {0};
  rb.Transition.pResource = state->rendertargets[frame];
  rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  rb.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
  rb.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
  state->list->lpVtbl->ResourceBarrier(state->list, 1, &rb);
    
  state->list->lpVtbl->Close(state->list);
  state->queue->lpVtbl->ExecuteCommandLists(state->queue, 1, (ID3D12CommandList **)&state->list);
  DXWaitForFence(state);
  state->swapchain->lpVtbl->Present(state->swapchain, 1, 0);
}

DX12STATE DXInit(HWND window) {
  // Setup debug symbols and device
  ID3D12Debug *debug;
  ID3D12InfoQueue *infoqueue;
#ifdef SLINAPP_DEBUG
  D3D12GetDebugInterface(&IID_ID3D12Debug, &debug);
  if (debug) {
    debug->lpVtbl->EnableDebugLayer(debug);
  }

  unsigned int flags = DXGI_CREATE_FACTORY_DEBUG;
#else
  debug = 0;
  unsigned int flags = 0;
#endif

  IDXGIFactory4 *factory;
  CreateDXGIFactory2(flags, &IID_IDXGIFactory, &factory);

  IDXGIAdapter *adapter;
  ID3D12Device *device;
  for (unsigned int i = 0; factory->lpVtbl->EnumAdapters(factory, i, &adapter) != DXGI_ERROR_NOT_FOUND; i++) {
    if (D3D12CreateDevice((IUnknown *)adapter, D3D_FEATURE_LEVEL_12_0, &IID_ID3D12Device, 0) == S_FALSE) {
      break;
    }
  }

  D3D12CreateDevice((IUnknown *)adapter, D3D_FEATURE_LEVEL_12_0, &IID_ID3D12Device, &device);

#ifdef SLINAPP_DEBUG
  device->lpVtbl->QueryInterface(device, &IID_ID3D12InfoQueue, &infoqueue);
  infoqueue->lpVtbl->SetBreakOnSeverity(infoqueue, D3D12_MESSAGE_SEVERITY_CORRUPTION, 1);
  infoqueue->lpVtbl->SetBreakOnSeverity(infoqueue, D3D12_MESSAGE_SEVERITY_WARNING, 1);
  infoqueue->lpVtbl->SetBreakOnSeverity(infoqueue, D3D12_MESSAGE_SEVERITY_ERROR, 1);
#else
  infoqueue = 0;
#endif
  
  // Setup command symbols
  D3D12_COMMAND_QUEUE_DESC cqd = {0};
  ID3D12CommandQueue *queue;
  device->lpVtbl->CreateCommandQueue(device, &cqd, &IID_ID3D12CommandQueue, &queue);

  DXGI_SWAP_CHAIN_DESC1 scd = {0};
  scd.Width = WINDOW_WIDTH;
  scd.Height = WINDOW_HEIGHT;
  scd.BufferCount = 2;
  scd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  scd.SampleDesc.Count = 1;
  IDXGISwapChain3 *swapchain;
  factory->lpVtbl->CreateSwapChainForHwnd(factory, (IUnknown *)queue, window, &scd, 0, 0, (IDXGISwapChain1 **)&swapchain);

  ID3D12CommandAllocator *allocator;
  ID3D12GraphicsCommandList *list;
  ID3D12Fence *fence;
  device->lpVtbl->CreateCommandAllocator(device, 0, &IID_ID3D12CommandAllocator, &allocator);
  device->lpVtbl->CreateCommandList(device, 0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator, 0, &IID_ID3D12CommandList, &list);
  list->lpVtbl->Close(list);

  device->lpVtbl->CreateFence(device, 0, 0, &IID_ID3D12Fence, &fence);
  HANDLE fenceevent = CreateEvent(0, 0, 0, 0);

  // Setup render targets
  D3D12_DESCRIPTOR_HEAP_DESC dhd = {0};
  dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  dhd.NumDescriptors = 2;
  ID3D12DescriptorHeap *rtvheap;
  device->lpVtbl->CreateDescriptorHeap(device, &dhd, &IID_ID3D12DescriptorHeap, &rtvheap);

  D3D12_CPU_DESCRIPTOR_HANDLE cpudescriptor = DXGetCPUDescriptorHandleForHeapStart(rtvheap);
  unsigned int rtvDescriptorSize = device->lpVtbl->GetDescriptorHandleIncrementSize(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  ID3D12Resource *rendertargets[2];
  for (unsigned int i = 0; i < 2; i++) {
    swapchain->lpVtbl->GetBuffer(swapchain, i, &IID_ID3D12Resource, &rendertargets[i]);
    device->lpVtbl->CreateRenderTargetView(device, rendertargets[i], 0, cpudescriptor);
    cpudescriptor.ptr += rtvDescriptorSize;
  }

  // Setup depth stencils
  ID3D12DescriptorHeap *dsvheap;
  {
    dhd = (D3D12_DESCRIPTOR_HEAP_DESC){0};
    dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dhd.NumDescriptors = 1;
    device->lpVtbl->CreateDescriptorHeap(device, &dhd, &IID_ID3D12DescriptorHeap, &dsvheap);

    D3D12_HEAP_PROPERTIES hp = {0};
    hp.Type = D3D12_HEAP_TYPE_DEFAULT;
    hp.CreationNodeMask = 1;
    hp.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC rc = {0};
    rc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    rc.Width = WINDOW_WIDTH;
    rc.Height = WINDOW_HEIGHT;
    rc.DepthOrArraySize = 1;
    rc.MipLevels = 1;
    rc.SampleDesc.Count = 1;
    rc.Format = DXGI_FORMAT_D32_FLOAT;
    rc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE cv = {0};
    cv.Format = DXGI_FORMAT_D32_FLOAT;
    cv.DepthStencil.Depth = 1.0f;

    ID3D12Resource *dsresource;
    device->lpVtbl->CreateCommittedResource(device, &hp, 0, &rc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &cv, &IID_ID3D12Resource, &dsresource);

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvd = {0};
    dsvd.Format = DXGI_FORMAT_D32_FLOAT;
    dsvd.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

    device->lpVtbl->CreateDepthStencilView(device, dsresource, &dsvd, DXGetCPUDescriptorHandleForHeapStart(dsvheap));
  }

  // Setup resources
  DX12STATE state = {0};
  state.dsvheap = dsvheap;
  state.debug = debug;
  state.factory = factory;
  state.adapter = adapter;
  state.device = device;
  state.queue = queue;
  state.infoqueue = infoqueue;
  state.swapchain = swapchain;
  state.rtvheap = rtvheap;
  state.rendertargets[0] = rendertargets[0];
  state.rendertargets[1] = rendertargets[1];
  state.allocator = allocator;
  state.list = list;
  state.fence = fence;
  state.fenceevent = fenceevent;
  DXWaitForFence(&state); // TODO tmp cbuffer
  return state;
}

DXSHADER DXCreateShader(DX12STATE *state, unsigned int cbcount, unsigned int cbsizes[], void *vcode, unsigned int vsize, void *pcode, unsigned int psize, D3D12_INPUT_ELEMENT_DESC ieds[], unsigned int iedcount) {
  DXSHADER shader = {0};

  D3D12_DESCRIPTOR_RANGE1 dr = {0};
  dr.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
  dr.NumDescriptors = cbcount; // TODO did i do this right?
  dr.BaseShaderRegister = 0;
  dr.RegisterSpace = 0;
  dr.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC;
  dr.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

  D3D12_ROOT_PARAMETER1 rootparameter = {0};
  rootparameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  rootparameter.DescriptorTable.NumDescriptorRanges = 1;
  rootparameter.DescriptorTable.pDescriptorRanges = &dr;
  rootparameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//D3D12_SHADER_VISIBILITY_VERTEX;

  D3D12_VERSIONED_ROOT_SIGNATURE_DESC vrsd = {0};
  vrsd.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
  vrsd.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;// | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
  vrsd.Desc_1_1.NumParameters = 1;
  vrsd.Desc_1_1.pParameters = &rootparameter;

  ID3DBlob *signature;
  D3D12SerializeVersionedRootSignature(&vrsd, &signature, 0);
  
  ID3D12RootSignature *rootsignature;
  state->device->lpVtbl->CreateRootSignature(state->device, 0, signature->lpVtbl->GetBufferPointer(signature), signature->lpVtbl->GetBufferSize(signature), &IID_ID3D12RootSignature, &rootsignature);

  D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsd = {0};
  gpsd.pRootSignature = rootsignature;
  gpsd.VS.pShaderBytecode = vcode;
  gpsd.VS.BytecodeLength = vsize;
  gpsd.PS.pShaderBytecode = pcode;
  gpsd.PS.BytecodeLength = psize;
  gpsd.SampleMask = UINT_MAX;
  gpsd.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
  gpsd.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
  gpsd.RasterizerState.FrontCounterClockwise = 0; // TODO this maybe
  gpsd.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
  gpsd.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
  gpsd.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
  gpsd.RasterizerState.DepthClipEnable = 1;
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
  for (unsigned int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++) {
    gpsd.BlendState.RenderTarget[i] = (D3D12_RENDER_TARGET_BLEND_DESC){0, 0, D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD, D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD, D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL};
  }

  ID3D12PipelineState *pipeline;
  state->device->lpVtbl->CreateGraphicsPipelineState(state->device, &gpsd, &IID_ID3D12PipelineState, &pipeline);

  D3D12_DESCRIPTOR_HEAP_DESC dhd = {0};
  dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  dhd.NumDescriptors = cbcount; // TODO did i do this right
  dhd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  ID3D12DescriptorHeap *cbvheap;
  state->device->lpVtbl->CreateDescriptorHeap(state->device, &dhd, &IID_ID3D12DescriptorHeap, &cbvheap);

  D3D12_HEAP_PROPERTIES hp = {0};
  hp.Type = D3D12_HEAP_TYPE_UPLOAD;
  hp.CreationNodeMask = 1;
  hp.VisibleNodeMask = 1;

  for (unsigned int i = 0; i < cbcount; i++) {
    D3D12_RESOURCE_DESC rc = {0};
    rc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    rc.Width = cbsizes[i];
    rc.Height = 1;
    rc.DepthOrArraySize = 1;
    rc.MipLevels = 1;
    rc.SampleDesc.Count = 1;
    rc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  
    ID3D12Resource *cbresource;
    state->device->lpVtbl->CreateCommittedResource(state->device, &hp, 0, &rc, D3D12_RESOURCE_STATE_GENERIC_READ, 0, &IID_ID3D12Resource, &cbresource);
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvd = {0};
    cbvd.BufferLocation = cbresource->lpVtbl->GetGPUVirtualAddress(cbresource);
    cbvd.SizeInBytes = cbsizes[i];
    state->device->lpVtbl->CreateConstantBufferView(state->device, &cbvd, DXGetCPUDescriptorHandleForHeapStart(cbvheap));
  
    void *ptr;
    D3D12_RANGE range = {0};
    cbresource->lpVtbl->Map(cbresource, 0, &range, &ptr);
    shader.cbptrs[i] = ptr;
  }

  shader.cbvheap = cbvheap;
  shader.rootsignature = rootsignature;
  shader.pipeline = pipeline;
  return shader;
}

ID3D12Resource *DXCreateResource(DX12STATE *state, void *data, unsigned int totalsize) {
  D3D12_HEAP_PROPERTIES hp = {0};
  hp.Type = D3D12_HEAP_TYPE_UPLOAD;
  hp.CreationNodeMask = 1;
  hp.VisibleNodeMask = 1;

  D3D12_RESOURCE_DESC rd = {0};
  rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  rd.Width = totalsize;
  rd.Height = 1;
  rd.DepthOrArraySize = 1;
  rd.MipLevels = 1;
  rd.SampleDesc.Count = 1;
  rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  ID3D12Resource *buffer;
  state->device->lpVtbl->CreateCommittedResource(state->device, &hp, 0, &rd, D3D12_RESOURCE_STATE_GENERIC_READ, 0, &IID_ID3D12Resource, &buffer);

  void *ptr;
  D3D12_RANGE range = {0};
  buffer->lpVtbl->Map(buffer, 0, &range, &ptr);
  CopyMemory(ptr, data, totalsize);
  buffer->lpVtbl->Unmap(buffer, 0, 0);
  DXWaitForFence(state);
  return buffer;
}

DX12VERTEXBUFFER DXCreateVertexBuffer(DX12STATE *state, void *data, unsigned int stridesize, unsigned int totalsize) {
  ID3D12Resource *resource = DXCreateResource(state, data, totalsize);

  D3D12_VERTEX_BUFFER_VIEW vertexbufferview = {0};
  vertexbufferview.BufferLocation = resource->lpVtbl->GetGPUVirtualAddress(resource);
  vertexbufferview.StrideInBytes = stridesize;
  vertexbufferview.SizeInBytes = totalsize;

  DX12VERTEXBUFFER ret = {0};
  ret.buffer = resource;
  ret.view = vertexbufferview;
  return ret;
}

DX12INDEXBUFFER DXCreateIndexBuffer(DX12STATE *state, void *data, unsigned int totalsize) {
  ID3D12Resource *resource = DXCreateResource(state, data, totalsize);

  D3D12_INDEX_BUFFER_VIEW indexbufferview = {0};
  indexbufferview.BufferLocation = resource->lpVtbl->GetGPUVirtualAddress(resource);
  indexbufferview.SizeInBytes = totalsize;
  indexbufferview.Format = DXGI_FORMAT_R32_UINT;

  DX12INDEXBUFFER ret = {0};
  ret.buffer = resource;
  ret.view = indexbufferview;
  return ret;
}
