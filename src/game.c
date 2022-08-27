void InitialiseHeap0(WINSTATE *winstate) {
  DX12STATE *dxstate = &winstate->dxstate;

  // Calculate the starting byte of each asset in the heap, and hence the total size required
  UQWORD constantbuffer0offset = 0;
  UQWORD totalsize = AlignUp(constantbuffer0offset + sizeof(CB0) * 100, 1024 * 64);

  // Create normal descriptorheap and upload heap
  winstate->heap0.heap = DXCreateHeap(dxstate, totalsize, D3D12_HEAP_TYPE_DEFAULT);
  winstate->heap0.uploadheap = DXCreateHeap(dxstate, totalsize, D3D12_HEAP_TYPE_UPLOAD);

  // Create a constant buffers
  winstate->heap0.constantbuffer0 = DXCreateBufferResource(dxstate, sizeof(CB0) * 100, winstate->heap0.heap, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, constantbuffer0offset);
  winstate->heap0.constantbuffer0upload = DXCreateBufferResource(dxstate, sizeof(CB0) * 100, winstate->heap0.uploadheap, D3D12_RESOURCE_STATE_GENERIC_READ, constantbuffer0offset);

  // Create the constant buffer view using the descriptor heap
  D3D12_CONSTANT_BUFFER_VIEW_DESC constantbufferviewdesc = {0};
  constantbufferviewdesc.BufferLocation = winstate->heap0.constantbuffer0->lpVtbl->GetGPUVirtualAddress(winstate->heap0.constantbuffer0);
  constantbufferviewdesc.SizeInBytes = sizeof(CB0) * 100;

  dxstate->device->lpVtbl->CreateConstantBufferView(dxstate->device, &constantbufferviewdesc, DXGetCPUDescriptorHandleForHeapStart(winstate->descriptorheap));
}

void GameInit(WINSTATE *winstate) {
  DX12STATE *dxstate = &winstate->dxstate;

  // Create vertex shader and pixel shader
  D3D12_INPUT_ELEMENT_DESC ieds[] =
  {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
  };

  D3D12_INPUT_LAYOUT_DESC inputlayoutdesc = {0};
  inputlayoutdesc.pInputElementDescs = ieds;
  inputlayoutdesc.NumElements = SizeofArray(ieds);

  WINRESOURCE vertex = WINLoadResource(SHADER_VERTEX, HLSL);
  WINRESOURCE pixel = WINLoadResource(SHADER_PIXEL, HLSL);
  winstate->shader = DXCreateShader(dxstate, vertex.data, vertex.size, pixel.data, pixel.size, inputlayoutdesc);

  // Create sampler
  winstate->sampler = DXCreateSampler(dxstate);

  // Reset the command list and allocator in order to upload resources to heap0
  dxstate->allocator->lpVtbl->Reset(dxstate->allocator);
  dxstate->list->lpVtbl->Reset(dxstate->list, dxstate->allocator, winstate->shader.pipeline);

  // Create descriptor heaps for both heap0 and heap1
  UDWORD heap0descriptorheapcount = 1;
  UDWORD heap1descriptorheapcount = 10; // TODO awful

  D3D12_DESCRIPTOR_HEAP_DESC descriptorheapdesc = {0};
  descriptorheapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  descriptorheapdesc.NumDescriptors = heap0descriptorheapcount + heap1descriptorheapcount;
  descriptorheapdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

  dxstate->device->lpVtbl->CreateDescriptorHeap(dxstate->device, &descriptorheapdesc, &IID_ID3D12DescriptorHeap, &winstate->descriptorheap);

  // Initialise heap
  InitialiseHeap0(winstate);

  // Initialise world
  WINRESOURCE r = WINLoadResource(CHAO, MODEL); // TODO rename these defines (e.g WORLD1, WORLD)
  // TODO I think there's a problem with loading multiple worlds. The descriptor heaps suddenly don't line up (which is ok in the LoadGLTF call) but when accessing the descriptors in the SetRootDescriptorTables function, it's wrong
  // To be honest, i'm not sure. but i'll put this comment here just in case i forget in the future and something messes up and i don't know why

  D3D12_CPU_DESCRIPTOR_HANDLE cpudescriptorhandle = DXGetCPUDescriptorHandleForHeapStart(winstate->descriptorheap);
  cpudescriptorhandle.ptr += dxstate->device->lpVtbl->GetDescriptorHandleIncrementSize(dxstate->device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * heap0descriptorheapcount; // Offset heap0

  D3D12_GPU_DESCRIPTOR_HANDLE gpudescriptorhandle = DXGetGPUDescriptorHandleForHeapStart(winstate->descriptorheap);
  gpudescriptorhandle.ptr += dxstate->device->lpVtbl->GetDescriptorHandleIncrementSize(dxstate->device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * heap0descriptorheapcount; // Offset heap0

  winstate->world1 = LoadGLTF(winstate, r.data, r.size, cpudescriptorhandle, gpudescriptorhandle);

  // Close list, execute commands and wait
  dxstate->list->lpVtbl->Close(dxstate->list);
  dxstate->queue->lpVtbl->ExecuteCommandLists(dxstate->queue, 1, (ID3D12CommandList **)&dxstate->list);
  DXWaitForFence(dxstate);

  // TODO temporary, change colours
  for (UDWORD i = 0; i < SizeofArray(winstate->world1.models); i++) {
    VECCopy4f(&winstate->world1.models[i].colour, (VECTOR4F){0, 1, 1, 1});
  }
}

void GameRender(WINSTATE *winstate) {
  DX12STATE *dxstate = &winstate->dxstate;

  // Reset command list and allocator
  dxstate->allocator->lpVtbl->Reset(dxstate->allocator);
  dxstate->list->lpVtbl->Reset(dxstate->list, dxstate->allocator, winstate->shader.pipeline);

  // Set root signature
  dxstate->list->lpVtbl->SetGraphicsRootSignature(dxstate->list, winstate->shader.rootsignature);

  // Set descriptor heaps
  ID3D12DescriptorHeap *heaps[] = {winstate->descriptorheap, winstate->sampler.heap};
  dxstate->list->lpVtbl->SetDescriptorHeaps(dxstate->list, SizeofArray(heaps), heaps);

  // Set root descriptor tables
  D3D12_GPU_DESCRIPTOR_HANDLE constantbufferhandle = DXGetGPUDescriptorHandleForHeapStart(winstate->descriptorheap);
  D3D12_GPU_DESCRIPTOR_HANDLE texturehandle = constantbufferhandle;
  texturehandle.ptr += dxstate->device->lpVtbl->GetDescriptorHandleIncrementSize(dxstate->device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); // TODO heap0 heap descriptor count?

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

  // Draw model using vertex, texture, and normal buffers, and index buffer
  dxstate->list->lpVtbl->IASetPrimitiveTopology(dxstate->list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  // Setup constant buffers
  D3D12_RANGE range = {0};
  void *resourcedata;
  winstate->heap0.constantbuffer0upload->lpVtbl->Map(winstate->heap0.constantbuffer0upload, 0, &range, &resourcedata);

  WORLD *world = &winstate->world1;

  for (UDWORD i = 0; i < SizeofArray(world->objects); i++) {
    WORLDOBJECT *obj = &world->objects[i];
    if (!obj->modelindex) {
      continue;
    }

    WINMODEL *model = &world->models[obj->modelindex];

    // Update constant buffer
    CB0 data = {0};
    MPerspective(&data.perspective, (float)WINDOW_HEIGHT / (float)WINDOW_WIDTH, DegreesToRadians(100.0f), 0.1f, 100.0f);
    MTransform(&data.transform, obj->transform.position[0], obj->transform.position[1], obj->transform.position[2], obj->transform.rotation[0], obj->transform.rotation[1], obj->transform.rotation[2], obj->transform.scale[0], obj->transform.scale[1], obj->transform.scale[2]);
    MInverseTransform(&data.camera, winstate->player.transform.position[0], winstate->player.transform.position[1], winstate->player.transform.position[2], winstate->player.transform.rotation[0], winstate->player.transform.rotation[1], winstate->player.transform.rotation[2]);
    VECCopy4f(&data.colour, model->colour);

    CopyMemory((void *)((UQWORD)resourcedata + (sizeof(CB0) * i)), &data, sizeof(CB0));
  }

  // Reuse rb
  rb.Transition.pResource = winstate->heap0.constantbuffer0;
  rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  rb.Transition.StateBefore = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
  rb.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
  dxstate->list->lpVtbl->ResourceBarrier(dxstate->list, 1, &rb);

  dxstate->list->lpVtbl->CopyResource(dxstate->list, winstate->heap0.constantbuffer0, winstate->heap0.constantbuffer0upload);

  winstate->heap0.constantbuffer0upload->lpVtbl->Unmap(winstate->heap0.constantbuffer0upload, 0, &range);

  rb.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
  rb.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
  dxstate->list->lpVtbl->ResourceBarrier(dxstate->list, 1, &rb);

  // Render all world objects
  for (UDWORD i = 0; i < SizeofArray(world->objects); i++) {
    WORLDOBJECT *obj = &world->objects[i];
    if (!obj->modelindex) {
      continue;
    }

    WINMODEL *model = &world->models[obj->modelindex];

    D3D12_VERTEX_BUFFER_VIEW views[3] = {
      model->vertexbuffer.view,
      model->texturebuffer.view,
      model->normalbuffer.view
    };

    if (model->textureindex) {
      dxstate->list->lpVtbl->SetGraphicsRootDescriptorTable(dxstate->list, 2, world->texture[model->textureindex].handle);
    }

    dxstate->list->lpVtbl->SetGraphicsRoot32BitConstant(dxstate->list, 1, i, 0);

    dxstate->list->lpVtbl->IASetVertexBuffers(dxstate->list, 0, SizeofArray(views), views);
    dxstate->list->lpVtbl->IASetIndexBuffer(dxstate->list, &model->indexbuffer.view);
    dxstate->list->lpVtbl->DrawIndexedInstanced(dxstate->list, model->facecount * 3, 1, 0, 0, 0);
  }

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
  PLAYER *player = &winstate->player;
  TRANSFORM *transform = &player->transform;

  // Parse look movements
  VECTOR3F look = {winstate->controls.look[1],  winstate->controls.look[0], 0};
  float lookmag = VECMagnitude3f(look);
  if (lookmag > 1.0f) {
    VECNormalise3f(&look);
  } else if (lookmag < 0.1f) {
    VECIdentity3f(&look);
  }

  VECMul3f(&look, (VECTOR3F){0.1f, 0.1f, 0});
  VECAdd3f(&transform->rotation, look);

  // Parse move movements
  VECTOR2F move;
  VECCopy2f(&move, winstate->controls.move);
  float movemag = VECMagnitude2f(move);
  if (movemag > 1.0f) {
    VECNormalise2f(&move);
  } else if (movemag < 0.1f) {
    VECIdentity2f(&move);
  }

  VECMul2f(&move, (VECTOR2F){0.1f, 0.1f});

  float rotcos = cosf(winstate->player.transform.rotation[1]);
  float rotsin = sinf(winstate->player.transform.rotation[1]);

  VECTOR3F movedir = {0};
  movedir[0] = move[0] * rotcos - move[1] * rotsin;
  movedir[2] = -move[0] * rotsin - move[1] * rotcos;

  VECAdd3f(&winstate->player.transform.position, movedir);

  if (winstate->controls.actions & ACTION_JUMP) {
    winstate->player.velocity[1] = 1;
  }

  VECAdd3f(&transform->position, winstate->player.velocity);

  if (winstate->player.transform.position[1] < 0) {
     winstate->player.transform.position[1] = 0;
  }

  winstate->player.velocity[1] -= 0.05f;
  

  GameRender(winstate);
}