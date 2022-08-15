void GameInit(WINSTATE *winstate) {
  DX12STATE *dxstate = &winstate->dxstate;

  D3D12_INPUT_ELEMENT_DESC ieds[] =
  {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
  };

  WINRESOURCE vertex = WINLoadResource(SHADER_VERTEX, HLSL);
  WINRESOURCE pixel = WINLoadResource(SHADER_PIXEL, HLSL);
  winstate->shader = DXCreateShader(dxstate, vertex.data, vertex.size, pixel.data, pixel.size, ieds, SizeofArray(ieds));

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

  {
    // Calculate the starting byte of each asset in the heap, and hence the total size required
    UQWORD constantbuffer0offset = 0;
    UQWORD totalsize = AlignUp(constantbuffer0offset + pixel.size, 1024 * 64);
  
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

  // --- Heap 1
  {
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
    D3D12_RESOURCE_DESC resourcedesc = {0};
    resourcedesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourcedesc.Width = width;
    resourcedesc.Height = height;
    resourcedesc.DepthOrArraySize = 1;
    resourcedesc.MipLevels = 1;
    resourcedesc.SampleDesc.Count = 1;
    resourcedesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    ID3D12Resource *texture, *textureupload;
    dxstate->device->lpVtbl->CreatePlacedResource(dxstate->device, dxstate->heap1.heap, textureoffset, &resourcedesc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0, &IID_ID3D12Resource, &texture);
    
    resourcedesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourcedesc.Width = AlignUp(width, 256) * height * 4;
    resourcedesc.Height = 1;
    resourcedesc.Format = DXGI_FORMAT_UNKNOWN;
    resourcedesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    dxstate->device->lpVtbl->CreatePlacedResource(dxstate->device, dxstate->heap1.uploadheap, textureoffset, &resourcedesc, D3D12_RESOURCE_STATE_GENERIC_READ, 0, &IID_ID3D12Resource, &textureupload);

    void *ptr;
    D3D12_RANGE range = {0};
    textureupload->lpVtbl->Map(textureupload, 0, &range, &ptr);

    D3D12_TEXTURE_COPY_LOCATION dst = {0};
    dst.pResource = texture;

    D3D12_TEXTURE_COPY_LOCATION src = {0};
    src.pResource = textureupload;
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    src.PlacedFootprint.Footprint.Width = width;
    src.PlacedFootprint.Footprint.Height = height;
    src.PlacedFootprint.Footprint.Depth = 1;
    src.PlacedFootprint.Footprint.RowPitch = AlignUp(width * 4, 256); // TODO no
    src.PlacedFootprint.Offset = AlignUp((UQWORD)ptr, 512) - (UQWORD)ptr;

    for (SDWORD i = 0; i < height; i++) {
      void *srcptr = (void *)((UQWORD)imgdata + (UQWORD)(width * i * 4));
      void *destptr = (void *)((UQWORD)ptr + src.PlacedFootprint.Offset + (UQWORD)(src.PlacedFootprint.Footprint.RowPitch * i));
      CopyMemory(destptr, srcptr, width * 4);
    }

    textureupload->lpVtbl->Unmap(textureupload, 0, 0);

    D3D12_RESOURCE_BARRIER rb = {0};
    rb.Transition.pResource = texture;
    rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    rb.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    rb.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    dxstate->list->lpVtbl->ResourceBarrier(dxstate->list, 1, &rb);

    dxstate->list->lpVtbl->CopyTextureRegion(dxstate->list, &dst, 0, 0, 0, &src, 0);

    rb.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    rb.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    dxstate->list->lpVtbl->ResourceBarrier(dxstate->list, 1, &rb);

    // Create the shader buffer view using the descriptor heap
    D3D12_SHADER_RESOURCE_VIEW_DESC srvdesc = {0};
    srvdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvdesc.Texture2D.MipLevels = 1;

    D3D12_CPU_DESCRIPTOR_HANDLE cpudescriptorhandle = DXGetCPUDescriptorHandleForHeapStart(dxstate->descriptorheap);
    cpudescriptorhandle.ptr += dxstate->device->lpVtbl->GetDescriptorHandleIncrementSize(dxstate->device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * heap0descriptorheapcount; // Offset heap0

    dxstate->device->lpVtbl->CreateShaderResourceView(dxstate->device, texture, &srvdesc, cpudescriptorhandle);

    // TODO move this somewhere appropriate
    //cgltf_free(data);
  }

  dxstate->list->lpVtbl->Close(dxstate->list);
  dxstate->queue->lpVtbl->ExecuteCommandLists(dxstate->queue, 1, (ID3D12CommandList **)&dxstate->list);
  DXWaitForFence(dxstate);
}

void GameUpdate(WINSTATE *winstate) {
  DX12STATE *dxstate = &winstate->dxstate;

  DXEnableShader(dxstate, winstate->shader);
  DXPrepareFrame(dxstate);
  
  VECTOR2F look;
  VECCopy2f(&look, winstate->controls.look);
  if (VECMagnitude2f(look) < 0.1f) {
    VECIdentity(&look);
  }

  VECMul2f(&look, (VECTOR2F){0.1f, 0.1f});

  winstate->camera.rotation[1] += look[0];
  winstate->camera.rotation[0] += look[1];

  VECTOR2F move;
  VECCopy2f(&move, winstate->controls.move);
  float mag = VECMagnitude2f(move);
  if (mag > 1.0f) {
    VECNormalise2f(&move);
  } else if (mag < 0.1f) {
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

  dxstate->list->lpVtbl->IASetPrimitiveTopology(dxstate->list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  SWOBJECT obj = {0};
  obj.position[2] = 5;

  D3D12_RANGE range = {0};

  void *resourcedata;
  dxstate->heap0.constantbuffer0upload->lpVtbl->Map(dxstate->heap0.constantbuffer0upload, 0, &range, &resourcedata);

  CB0 data = {0};
  MPerspective(&data.perspective, DegreesToRadians(100.0f), 0.1f, 100.0f);
  MTransform(&data.transform, 0, 0, 15, 0, 0, 0);
  MTransform(&data.camera, winstate->camera.position[0], winstate->camera.position[1], winstate->camera.position[2], winstate->camera.rotation[0], winstate->camera.rotation[1], winstate->camera.rotation[2]);

  CopyMemory(resourcedata, &data, sizeof(CB0));

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

  dxstate->list->lpVtbl->IASetVertexBuffers(dxstate->list, 0, 1, &winstate->models[obj.modelid].vertexbuffer.view);
  dxstate->list->lpVtbl->IASetIndexBuffer(dxstate->list, &winstate->models[obj.modelid].indexbuffer.view);
  dxstate->list->lpVtbl->DrawIndexedInstanced(dxstate->list, winstate->models[obj.modelid].facecount * 3, 1, 0, 0, 0);

  DXFlushFrame(dxstate);
}