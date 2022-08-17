void InitialiseHeap0(DX12STATE *dxstate) {
  // Calculate the starting byte of each asset in the heap, and hence the total size required
  UQWORD constantbuffer0offset = 0;
  UQWORD totalsize = AlignUp(constantbuffer0offset + sizeof(CB0) * 100, 1024 * 64);

  // Create normal descriptorheap and upload heap
  dxstate->heap0.heap = DXCreateHeap(dxstate, totalsize, D3D12_HEAP_TYPE_DEFAULT);
  dxstate->heap0.uploadheap = DXCreateHeap(dxstate, totalsize, D3D12_HEAP_TYPE_UPLOAD);

  // Create a constant buffers
  dxstate->heap0.constantbuffer0 = DXCreateBufferResource(dxstate, sizeof(CB0) * 100, dxstate->heap0.heap, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, constantbuffer0offset);
  dxstate->heap0.constantbuffer0upload = DXCreateBufferResource(dxstate, sizeof(CB0) * 100, dxstate->heap0.uploadheap, D3D12_RESOURCE_STATE_GENERIC_READ, constantbuffer0offset);

  // Create the constant buffer view using the descriptor heap
  D3D12_CONSTANT_BUFFER_VIEW_DESC constantbufferviewdesc = {0};
  constantbufferviewdesc.BufferLocation = dxstate->heap0.constantbuffer0->lpVtbl->GetGPUVirtualAddress(dxstate->heap0.constantbuffer0);
  constantbufferviewdesc.SizeInBytes = sizeof(CB0) * 100;

  dxstate->device->lpVtbl->CreateConstantBufferView(dxstate->device, &constantbufferviewdesc, DXGetCPUDescriptorHandleForHeapStart(dxstate->descriptorheap));
}

void InitialiseHeap1(WINSTATE *winstate, UDWORD heap0descriptorheapcount) {
  DX12STATE *dxstate = &winstate->dxstate;

  // Load all models
  WINRESOURCE r = WINLoadResource(CHAO, MODEL);

  cgltf_options options = {0};
  cgltf_data* bufdata;
  cgltf_parse(&options, r.data, r.size, &bufdata);
  cgltf_load_buffers(&options, bufdata, 0);

  // Load image
  WINRESOURCE tex = WINLoadResource(IMAGE, PNG);
  SDWORD width, height, n;

  void *imgdata = stbi_load_from_memory(tex.data, tex.size, &width, &height, &n, 0);

  UQWORD heaptotalsize = 0; // This variable is to store the size of the WHOLE allocated heap (to be used later when allocating memory for the heap)
  for (UDWORD i = 0; i < bufdata->meshes_count; i++) {
    cgltf_buffer_view* indexbuffer = bufdata->meshes[i].primitives->indices->buffer_view;
    cgltf_buffer_view* vertexbuffer = bufdata->meshes[i].primitives->attributes[0].data->buffer_view;
    cgltf_buffer_view* texturebuffer = bufdata->meshes[i].primitives->attributes[2].data->buffer_view;
    cgltf_buffer_view* normalbuffer = bufdata->meshes[i].primitives->attributes[1].data->buffer_view;

    UQWORD vertexbufferoffset = 0;
    UQWORD texturebufferoffset = AlignUp(vertexbufferoffset + vertexbuffer->size, 1024 * 64);
    UQWORD normalbufferoffset = AlignUp(texturebufferoffset + texturebuffer->size, 1024 * 64);
    UQWORD indexbufferoffset = AlignUp(normalbufferoffset + normalbuffer->size, 1024 * 64);
    heaptotalsize += AlignUp(indexbufferoffset + indexbuffer->size, 1024 * 64);
  }

  // Make sure to add the texture size
  heaptotalsize += AlignUp(width, 256) * height * 4; // TODO make this into a function? this is a goofy equation and i'd never remember it

  // Create normal heap and upload heap
  dxstate->heap1.heap = DXCreateHeap(dxstate, heaptotalsize, D3D12_HEAP_TYPE_DEFAULT);
  dxstate->heap1.uploadheap = DXCreateHeap(dxstate, heaptotalsize, D3D12_HEAP_TYPE_UPLOAD);

  UQWORD totalallocatedsize = 0;  // This variable is to store the total size used up (so far) after each model is allocated.
                                  // This is in order to ensure the correct offset and no overlapping
  for (UDWORD i = 0; i < bufdata->meshes_count; i++) {
    // Calculate the starting byte of each asset in the descriptorheap, and hence the total size required
    cgltf_buffer_view* indexbuffer = bufdata->meshes[i].primitives->indices->buffer_view;
    cgltf_buffer_view* vertexbuffer = bufdata->meshes[i].primitives->attributes[0].data->buffer_view;
    cgltf_buffer_view* texturebuffer = bufdata->meshes[i].primitives->attributes[2].data->buffer_view;
    cgltf_buffer_view* normalbuffer = bufdata->meshes[i].primitives->attributes[1].data->buffer_view;

    UQWORD vertexbufferoffset = totalallocatedsize;
    UQWORD texturebufferoffset = AlignUp(vertexbufferoffset + vertexbuffer->size, 1024 * 64);
    UQWORD normalbufferoffset = AlignUp(texturebufferoffset + texturebuffer->size, 1024 * 64);
    UQWORD indexbufferoffset = AlignUp(normalbufferoffset + normalbuffer->size, 1024 * 64);
    UQWORD totalsize = AlignUp(indexbufferoffset + indexbuffer->size, 1024 * 64);

    // Increment offset for the next model
    totalallocatedsize = totalsize;

    // Create vertex, texture, and normal buffers, and index buffer for model
    winstate->models[i].vertexbuffer = DXCreateAndUploadVertexBuffer(dxstate, dxstate->heap1.heap, dxstate->heap1.uploadheap, (void*)((UQWORD)vertexbuffer->buffer->data + (UQWORD)vertexbuffer->offset), vertexbuffer->size, vertexbufferoffset, 3 * sizeof(float));

    winstate->models[i].texturebuffer = DXCreateAndUploadVertexBuffer(dxstate, dxstate->heap1.heap, dxstate->heap1.uploadheap, (void*)((UQWORD)texturebuffer->buffer->data + (UQWORD)texturebuffer->offset), texturebuffer->size, texturebufferoffset, 2 * sizeof(float));

    winstate->models[i].normalbuffer = DXCreateAndUploadVertexBuffer(dxstate, dxstate->heap1.heap, dxstate->heap1.uploadheap, (void*)((UQWORD)normalbuffer->buffer->data + (UQWORD)normalbuffer->offset), normalbuffer->size, normalbufferoffset, 3 * sizeof(float));

    winstate->models[i].indexbuffer = DXCreateAndUploadIndexBuffer(dxstate, dxstate->heap1.heap, dxstate->heap1.uploadheap, (void*)((UQWORD)indexbuffer->buffer->data + (UQWORD)indexbuffer->offset), indexbuffer->size, indexbufferoffset, DXGI_FORMAT_R16_UINT);

    winstate->models[i].facecount = (UDWORD)indexbuffer->size / 2 / 3;
  }
  
  for (DWORD i = 0; i < bufdata->nodes_count; i++) {
    WORLDOBJECT *obj = &winstate->objects[i];
    VECCopy3f(obj->transform.position, bufdata->nodes[i].translation);
    VECCopy3f(obj->transform.rotation, bufdata->nodes[i].rotation);
    VECCopy3f(obj->transform.scale, bufdata->nodes[i].scale);
    obj->model = &winstate->models[((UQWORD)bufdata->nodes[i].mesh - (UQWORD)bufdata->meshes) / sizeof(cgltf_mesh)];
  }

  // Use the totalallocatedsize variable in order to offset the texture in the heap
  DX12TEXTURE texture = DXCreateAndUploadTexture(dxstate, width, height, dxstate->heap1.heap, dxstate->heap1.uploadheap, imgdata, totalallocatedsize);

  // Create the shader buffer view in the descriptor heap
  D3D12_SHADER_RESOURCE_VIEW_DESC srvdesc = {0};
  srvdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvdesc.Texture2D.MipLevels = 1;

  D3D12_CPU_DESCRIPTOR_HANDLE cpudescriptorhandle = DXGetCPUDescriptorHandleForHeapStart(dxstate->descriptorheap);
  cpudescriptorhandle.ptr += dxstate->device->lpVtbl->GetDescriptorHandleIncrementSize(dxstate->device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * heap0descriptorheapcount; // Offset heap0

  dxstate->device->lpVtbl->CreateShaderResourceView(dxstate->device, texture.texture, &srvdesc, cpudescriptorhandle);

  dxstate->sampler = DXCreateSampler(dxstate);

  // TODO cgltf_free(imgdata);
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

  // Reset the command list and allocator in order to upload resources to heap0 and heap1
  dxstate->allocator->lpVtbl->Reset(dxstate->allocator);
  dxstate->list->lpVtbl->Reset(dxstate->list, dxstate->allocator, winstate->shader.pipeline);

  // Create descriptor heaps for both heap0 and heap1
  UDWORD heap0descriptorheapcount = 1;
  UDWORD heap1descriptorheapcount = 1;

  D3D12_DESCRIPTOR_HEAP_DESC descriptorheapdesc = {0};
  descriptorheapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  descriptorheapdesc.NumDescriptors = heap0descriptorheapcount + heap1descriptorheapcount;
  descriptorheapdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

  dxstate->device->lpVtbl->CreateDescriptorHeap(dxstate->device, &descriptorheapdesc, &IID_ID3D12DescriptorHeap, &dxstate->descriptorheap);

  // Initialise heaps
  InitialiseHeap0(dxstate);
  InitialiseHeap1(winstate, heap0descriptorheapcount);

  // Close list, execute commands and wait
  dxstate->list->lpVtbl->Close(dxstate->list);
  dxstate->queue->lpVtbl->ExecuteCommandLists(dxstate->queue, 1, (ID3D12CommandList **)&dxstate->list);
  DXWaitForFence(dxstate);
}

void GameRender(WINSTATE *winstate) {
  DX12STATE *dxstate = &winstate->dxstate;

  // Reset command list and allocator
  dxstate->allocator->lpVtbl->Reset(dxstate->allocator);
  dxstate->list->lpVtbl->Reset(dxstate->list, dxstate->allocator, winstate->shader.pipeline);

  // Set root signature
  dxstate->list->lpVtbl->SetGraphicsRootSignature(dxstate->list, winstate->shader.rootsignature);

  // Set descriptor heaps
  ID3D12DescriptorHeap *heaps[] = {dxstate->descriptorheap, dxstate->sampler.heap};
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

  // Draw model using vertex, texture, and normal buffers, and index buffer
  dxstate->list->lpVtbl->IASetPrimitiveTopology(dxstate->list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  // Setup constant buffers
  D3D12_RANGE range = {0};
  void *resourcedata;
  dxstate->heap0.constantbuffer0upload->lpVtbl->Map(dxstate->heap0.constantbuffer0upload, 0, &range, &resourcedata);

  for (UDWORD i = 0; i < SizeofArray(winstate->objects); i++) {
    WORLDOBJECT *obj = &winstate->objects[i];
    if (!obj->model) {
      continue;
    }

    // Update constant buffer
    CB0 data = {0};
    MPerspective(&data.perspective, 1080.0f / 1920.0f, DegreesToRadians(100.0f), 0.1f, 100.0f);
    MTransform(&data.transform, obj->transform.position[0], obj->transform.position[1], obj->transform.position[2], obj->transform.rotation[0], obj->transform.rotation[1], obj->transform.rotation[2], obj->transform.scale[0], obj->transform.scale[1], obj->transform.scale[2]);
    MTransform(&data.camera, winstate->camera.position[0], winstate->camera.position[1], winstate->camera.position[2], winstate->camera.rotation[0], winstate->camera.rotation[1], winstate->camera.rotation[2], 1, 1, 1);

    CopyMemory((void *)((UQWORD)resourcedata + (sizeof(CB0) * i)), &data, sizeof(CB0));
  }

  // Reuse rb
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

  // Render all world objects
  for (UDWORD i = 0; i < SizeofArray(winstate->objects); i++) {
    WORLDOBJECT *obj = &winstate->objects[i];
    if (!obj->model) {
      continue;
    }

    WINMODEL *model = obj->model;

    D3D12_VERTEX_BUFFER_VIEW views[3] = {
      model->vertexbuffer.view,
      model->texturebuffer.view,
      model->normalbuffer.view
    };

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

  GameRender(winstate);
}