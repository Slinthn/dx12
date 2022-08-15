void InitialiseHeap0(DX12STATE *dxstate, UDWORD totaldescriptorheapcount) {
  // Create descriptor heaps for both heap0 and heap1

  D3D12_DESCRIPTOR_HEAP_DESC descriptorheapdesc = {0};
  descriptorheapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  descriptorheapdesc.NumDescriptors = totaldescriptorheapcount;
  descriptorheapdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

  dxstate->device->lpVtbl->CreateDescriptorHeap(dxstate->device, &descriptorheapdesc, &IID_ID3D12DescriptorHeap, &dxstate->descriptorheap);

  // Calculate the starting byte of each asset in the heap, and hence the total size required
  UQWORD constantbuffer0offset = 0;
  UQWORD totalsize = AlignUp(constantbuffer0offset, 1024 * 64);

  // Create normal descriptorheap and upload heap
  dxstate->heap0.heap = DXCreateHeap(dxstate, totalsize, D3D12_HEAP_TYPE_DEFAULT);
  dxstate->heap0.uploadheap = DXCreateHeap(dxstate, totalsize, D3D12_HEAP_TYPE_UPLOAD);

  // Create a constant buffers
  dxstate->heap0.constantbuffer0 = DXCreateBufferResource(dxstate, sizeof(CB0) * 100, dxstate->heap0.heap,D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,constantbuffer0offset);
  dxstate->heap0.constantbuffer0upload = DXCreateBufferResource(dxstate, sizeof(CB0) * 100, dxstate->heap0.uploadheap,D3D12_RESOURCE_STATE_GENERIC_READ,constantbuffer0offset);

  // Create the constant buffer view using the descriptor heap
  D3D12_CONSTANT_BUFFER_VIEW_DESC constantbufferviewdesc = {0};
  constantbufferviewdesc.BufferLocation = dxstate->heap0.constantbuffer0->lpVtbl->GetGPUVirtualAddress(dxstate->heap0.constantbuffer0);
  constantbufferviewdesc.SizeInBytes = sizeof(CB0) * 100;

  dxstate->device->lpVtbl->CreateConstantBufferView(dxstate->device, &constantbufferviewdesc, DXGetCPUDescriptorHandleForHeapStart(dxstate->descriptorheap));
}

void InitialiseHeap1(WINSTATE *winstate, UDWORD heap0descriptorheapcount) {
  DX12STATE *dxstate = &winstate->dxstate;

  // Load vertex and index buffers
  WINRESOURCE r = WINLoadResource(CHAO, MODEL);

  cgltf_options options = {0};
  cgltf_data* bufdata = 0;
  cgltf_result result = cgltf_parse(&options, r.data, r.size, &bufdata);
  if (result != cgltf_result_success)
    DebugBreak(); // hol up (literally)

  cgltf_load_buffers(&options, bufdata, 0);

  cgltf_buffer_view* indexbuffer = bufdata->buffer_views;
  cgltf_buffer_view* vertexbuffer = &bufdata->buffer_views[1];

  // Load image
  WINRESOURCE tex = WINLoadResource(IMAGE, PNG);
  SDWORD width, height, n;

  void *imgdata = stbi_load_from_memory(tex.data, tex.size, &width, &height, &n, 0);

  // Calculate the starting byte of each asset in the descriptorheap, and hence the total size required
  UQWORD vertexbufferoffset = 0;
  UQWORD indexbufferoffset = AlignUp(vertexbuffer->size, 1024 * 64);
  UQWORD textureoffset = AlignUp(indexbufferoffset + indexbuffer->size, 1024 * 64);
  UQWORD totalsize = AlignUp(textureoffset + (AlignUp(width, 256) * height * 4), 1024 * 64);

  // Create normal heap and upload heap
  dxstate->heap1.heap = DXCreateHeap(dxstate, totalsize, D3D12_HEAP_TYPE_DEFAULT);
  dxstate->heap1.uploadheap = DXCreateHeap(dxstate, totalsize, D3D12_HEAP_TYPE_UPLOAD);

  // Create vertex and index buffer for model
  winstate->models[0].vertexbuffer = DXCreateAndUploadVertexBuffer(dxstate, dxstate->heap1.heap, dxstate->heap1.uploadheap, (void*)((UQWORD)vertexbuffer->buffer->data + (UQWORD)vertexbuffer->offset), vertexbuffer->size, vertexbufferoffset);

  winstate->models[0].indexbuffer = DXCreateAndUploadIndexBuffer(dxstate, dxstate->heap1.heap, dxstate->heap1.uploadheap, (void*)((UQWORD)indexbuffer->buffer->data + (UQWORD)indexbuffer->offset), indexbuffer->size, indexbufferoffset, DXGI_FORMAT_R16_UINT);

  winstate->models[0].facecount = (UDWORD)indexbuffer->size / 2 / 3;

  // TODO texture code under here
  DX12TEXTURE texture = DXCreateAndUploadTexture(dxstate, width, height, dxstate->heap1.heap, dxstate->heap1.uploadheap, imgdata, textureoffset);

  // Create the shader buffer view using the descriptor heap
  D3D12_SHADER_RESOURCE_VIEW_DESC srvdesc = {0};
  srvdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvdesc.Texture2D.MipLevels = 1;

  D3D12_CPU_DESCRIPTOR_HANDLE cpudescriptorhandle = DXGetCPUDescriptorHandleForHeapStart(dxstate->descriptorheap);
  cpudescriptorhandle.ptr += dxstate->device->lpVtbl->GetDescriptorHandleIncrementSize(dxstate->device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * heap0descriptorheapcount; // Offset heap0

  dxstate->device->lpVtbl->CreateShaderResourceView(dxstate->device, texture.texture, &srvdesc, cpudescriptorhandle);

  // Create sampler
  D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {0};
  samplerHeapDesc.NumDescriptors = 1;
  samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
  samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

  ID3D12DescriptorHeap *samplerheap;
  dxstate->device->lpVtbl->CreateDescriptorHeap(dxstate->device, &samplerHeapDesc, &IID_ID3D12DescriptorHeap, &samplerheap);

  D3D12_SAMPLER_DESC samplerdesc = {0};
  samplerdesc.Filter = D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
  samplerdesc.AddressU = samplerdesc.AddressV = samplerdesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  samplerdesc.MaxLOD = D3D12_FLOAT32_MAX;
  samplerdesc.MaxAnisotropy = 1;
  samplerdesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;

  dxstate->device->lpVtbl->CreateSampler(dxstate->device, &samplerdesc, DXGetCPUDescriptorHandleForHeapStart(samplerheap));

  dxstate->samplerheap = samplerheap;

  // TODO cgltf_free(imgdata);
}

void GameInit(WINSTATE *winstate) {
  DX12STATE *dxstate = &winstate->dxstate;

  // Create vertex shader and pixel shader
  D3D12_INPUT_ELEMENT_DESC ieds[] =
  {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
  };

  WINRESOURCE vertex = WINLoadResource(SHADER_VERTEX, HLSL);
  WINRESOURCE pixel = WINLoadResource(SHADER_PIXEL, HLSL);
  winstate->shader = DXCreateShader(dxstate, vertex.data, vertex.size, pixel.data, pixel.size, ieds, SizeofArray(ieds));

  // Reset the command list and allocator in order to upload resources to heap0 and heap1
  dxstate->allocator->lpVtbl->Reset(dxstate->allocator);
  dxstate->list->lpVtbl->Reset(dxstate->list, dxstate->allocator, winstate->shader.pipeline);

  UDWORD heap0descriptorheapcount = 1;
  UDWORD heap1descriptorheapcount = 1;

  InitialiseHeap0(dxstate, heap0descriptorheapcount + heap1descriptorheapcount);
  InitialiseHeap1(winstate, heap0descriptorheapcount);

  // Close list, execute commands and wait
  dxstate->list->lpVtbl->Close(dxstate->list);
  dxstate->queue->lpVtbl->ExecuteCommandLists(dxstate->queue, 1, (ID3D12CommandList **)&dxstate->list);
  DXWaitForFence(dxstate);
}

void GameRender(WINSTATE *winstate) {
  DX12STATE *dxstate = &winstate->dxstate;

  // Set descriptor heaps
  ID3D12DescriptorHeap *heaps[] = {dxstate->descriptorheap, dxstate->samplerheap};
  dxstate->list->lpVtbl->SetDescriptorHeaps(dxstate->list, SizeofArray(heaps), heaps);

  // Set root descriptor tables
  D3D12_GPU_DESCRIPTOR_HANDLE constantbufferhandle = DXGetGPUDescriptorHandleForHeapStart(dxstate->descriptorheap);
  D3D12_GPU_DESCRIPTOR_HANDLE texturehandle = constantbufferhandle;
  texturehandle.ptr += dxstate->device->lpVtbl->GetDescriptorHandleIncrementSize(dxstate->device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

  dxstate->list->lpVtbl->SetGraphicsRootDescriptorTable(dxstate->list, 0, constantbufferhandle);
  dxstate->list->lpVtbl->SetGraphicsRootDescriptorTable(dxstate->list, 2, texturehandle);

  // Set pipeline state
  dxstate->list->lpVtbl->SetPipelineState(dxstate->list, winstate->shader.pipeline);

  // Get current frame
  UDWORD frame = dxstate->swapchain->lpVtbl->GetCurrentBackBufferIndex(dxstate->swapchain);

  // Setup viewport
  D3D12_VIEWPORT viewport = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, 1};
  dxstate->list->lpVtbl->RSSetViewports(dxstate->list, 1, &viewport);

  // Setup scissor
  D3D12_RECT scissor = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
  dxstate->list->lpVtbl->RSSetScissorRects(dxstate->list, 1, &scissor);

  // Set render target and depth target
  D3D12_CPU_DESCRIPTOR_HANDLE rtvdescriptor = DXGetCPUDescriptorHandleForHeapStart(dxstate->rendertargetviewdescriptorheap);
  rtvdescriptor.ptr += (UQWORD)frame * dxstate->device->lpVtbl->GetDescriptorHandleIncrementSize(dxstate->device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  D3D12_CPU_DESCRIPTOR_HANDLE dsvdescriptor = DXGetCPUDescriptorHandleForHeapStart(dxstate->depthstencilviewheap); 
  dxstate->list->lpVtbl->OMSetRenderTargets(dxstate->list, 1, &rtvdescriptor, 0, &dsvdescriptor);

  // Transition render target view to a drawing state
  D3D12_RESOURCE_BARRIER rb = {0};
  rb.Transition.pResource = dxstate->rendertargets[frame];
  rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  rb.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
  dxstate->list->lpVtbl->ResourceBarrier(dxstate->list, 1, &rb);

  // Clear buffers
  dxstate->list->lpVtbl->ClearRenderTargetView(dxstate->list, rtvdescriptor, (float[4]) {1.0f, 1.0f, 1.0f, 1.0f}, 0, 0);
  dxstate->list->lpVtbl->ClearDepthStencilView(dxstate->list, DXGetCPUDescriptorHandleForHeapStart(dxstate->depthstencilviewheap), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, 0);

  // Draw vertex buffer and index buffer
  dxstate->list->lpVtbl->IASetPrimitiveTopology(dxstate->list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  dxstate->list->lpVtbl->IASetVertexBuffers(dxstate->list, 0, 1, &winstate->models[0].vertexbuffer.view);
  dxstate->list->lpVtbl->IASetIndexBuffer(dxstate->list, &winstate->models[0].indexbuffer.view);
  dxstate->list->lpVtbl->DrawIndexedInstanced(dxstate->list, winstate->models[0].facecount * 3, 1, 0, 0, 0);

  // Transition render target view to a present state
  rb.Transition.pResource = dxstate->rendertargets[frame];
  rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  rb.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
  rb.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
  dxstate->list->lpVtbl->ResourceBarrier(dxstate->list, 1, &rb);

  // Close, execute command list and wait for GPU
  dxstate->list->lpVtbl->Close(dxstate->list);
  dxstate->queue->lpVtbl->ExecuteCommandLists(dxstate->queue, 1, (ID3D12CommandList **)&dxstate->list);
  DXWaitForFence(dxstate);

  // Present frame to the screen
  dxstate->swapchain->lpVtbl->Present(dxstate->swapchain, 1, 0);
}

void GameUpdate(WINSTATE *winstate) {
  DX12STATE *dxstate = &winstate->dxstate;
  
  // Parse look movements
  VECTOR2F look;
  VECCopy2f(&look, winstate->controls.look);
  float lookmag = VECMagnitude2f(look);
  if (lookmag > 1.0f) {
    VECNormalise2f(&look);
  } else if (lookmag < 0.1f) {
    VECIdentity(&look);
  }

  VECMul2f(&look, (VECTOR2F){0.1f, 0.1f});

  winstate->camera.rotation[1] += look[0];
  winstate->camera.rotation[0] += look[1];

  // Parse move movements
  VECTOR2F move;
  VECCopy2f(&move, winstate->controls.move);
  float movemag = VECMagnitude2f(move);
  if (movemag > 1.0f) {
    VECNormalise2f(&move);
  } else if (movemag < 0.1f) {
    VECIdentity(&move);
  }

  VECMul2f(&move, (VECTOR2F){0.1f, 0.1f});

  float rotcos = cosf(winstate->camera.rotation[1]);
  float rotsin = sinf(winstate->camera.rotation[1]);

  VECTOR3F movedir = {0};
  movedir[0] = move[0] * rotcos - move[1] * rotsin;
  movedir[2] = -move[0] * rotsin - move[1] * rotcos;

  if (winstate->controls.actions & ACTION_DESCEND) {
    movedir[1] -= 0.1f;
  }
  if (winstate->controls.actions & ACTION_ASCEND) {
    movedir[1] += 0.1f;
  }

  winstate->camera.position[0] += movedir[0];
  winstate->camera.position[1] += movedir[1];
  winstate->camera.position[2] += movedir[2];

  // Update constant buffer
  D3D12_RANGE range = {0};
  void *resourcedata;
  dxstate->heap0.constantbuffer0upload->lpVtbl->Map(dxstate->heap0.constantbuffer0upload, 0, &range, &resourcedata);

  static float rot = 0;
  rot += 0.01f;

  CB0 data = {0};
  MPerspective(&data.perspective, 1080.0f / 1920.0f, DegreesToRadians(100.0f), 0.1f, 100.0f);
  MTransform(&data.transform, 0, 0, 4, 0, rot, 0);
  MTransform(&data.camera, winstate->camera.position[0], winstate->camera.position[1], winstate->camera.position[2], winstate->camera.rotation[0], winstate->camera.rotation[1], winstate->camera.rotation[2]);

  CopyMemory(resourcedata, &data, sizeof(CB0));

  // Reset command list and allocator
  dxstate->allocator->lpVtbl->Reset(dxstate->allocator);
  dxstate->list->lpVtbl->Reset(dxstate->list, dxstate->allocator, winstate->shader.pipeline);

  // Set root signature
  dxstate->list->lpVtbl->SetGraphicsRootSignature(dxstate->list, winstate->shader.rootsignature);

  D3D12_RESOURCE_BARRIER rb = {0};
  rb.Transition.pResource = dxstate->heap0.constantbuffer0;
  rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  rb.Transition.StateBefore = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
  rb.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
  dxstate->list->lpVtbl->ResourceBarrier(dxstate->list, 1, &rb);

  dxstate->list->lpVtbl->CopyResource(dxstate->list, dxstate->heap0.constantbuffer0, dxstate->heap0.constantbuffer0upload);

  dxstate->heap0.constantbuffer0upload->lpVtbl->Unmap(dxstate->heap0.constantbuffer0upload, 0, &range);

  rb.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
  rb.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
  dxstate->list->lpVtbl->ResourceBarrier(dxstate->list, 1, &rb);

  dxstate->list->lpVtbl->SetGraphicsRoot32BitConstant(dxstate->list, 1, 0, 0);

  GameRender(winstate); // TODO assuming that command list is already reset
}