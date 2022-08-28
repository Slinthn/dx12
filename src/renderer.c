void RenderWorld(WINSTATE *winstate, BOOL coloured) {
  DX12STATE *dxstate = &winstate->dxstate;

  WORLD *world = &winstate->world1;

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

    if (coloured && model->textureindex) {
      dxstate->list->lpVtbl->SetGraphicsRootDescriptorTable(dxstate->list, 2, world->texture[model->textureindex].handle);
    }

    dxstate->list->lpVtbl->SetGraphicsRoot32BitConstant(dxstate->list, 1, i, 0);

    dxstate->list->lpVtbl->IASetVertexBuffers(dxstate->list, 0, SizeofArray(views), views);
    dxstate->list->lpVtbl->IASetIndexBuffer(dxstate->list, &model->indexbuffer.view);
    dxstate->list->lpVtbl->DrawIndexedInstanced(dxstate->list, model->facecount * 3, 1, 0, 0, 0);
  }
}

void RenderWholeShebang(WINSTATE *winstate, UDWORD frame) {
  DX12STATE *dxstate = &winstate->dxstate;

  // Setup CB2  
  {
    CB2 cb2 = {0};
    MPerspective(&cb2.perspective, (float)WINDOW_HEIGHT / (float)WINDOW_WIDTH, DegreesToRadians(100.0f), 0.1f, 100.0f);
    MInverseTransform(&cb2.camera, winstate->player.transform);


    D3D12_RANGE range = {0};
    void *resourcedata;
    winstate->heap0.constantbuffer2upload->lpVtbl->Map(winstate->heap0.constantbuffer2upload, 0, &range, &resourcedata);
    CopyMemory(resourcedata, &cb2, sizeof(CB2));
    winstate->heap0.constantbuffer2upload->lpVtbl->Unmap(winstate->heap0.constantbuffer2upload, 0, &range);

    // Reuse rb
    D3D12_RESOURCE_BARRIER rb = {0};
    rb.Transition.pResource = winstate->heap0.constantbuffer2;
    rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    rb.Transition.StateBefore = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    rb.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    dxstate->list->lpVtbl->ResourceBarrier(dxstate->list, 1, &rb);

    dxstate->list->lpVtbl->CopyResource(dxstate->list, winstate->heap0.constantbuffer2, winstate->heap0.constantbuffer2upload);

    rb.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    rb.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    dxstate->list->lpVtbl->ResourceBarrier(dxstate->list, 1, &rb);
  }

  // Setup CB0
  {
    WORLD *world = &winstate->world1;

    CB0 cb0 = {0};

    for (UDWORD i = 0; i < SizeofArray(world->objects); i++) {
      WORLDOBJECT *obj = &world->objects[i];
      if (!obj->modelindex) {
        continue;
      }

      WINMODEL *model = &world->models[obj->modelindex];

      // Update constant buffer
      MTransform(&cb0.data[i].transform, obj->transform);
      VECCopy4f(&cb0.data[i].colour, model->colour);
    }

    D3D12_RANGE range = {0};
    void *resourcedata;
    winstate->heap0.constantbuffer0upload->lpVtbl->Map(winstate->heap0.constantbuffer0upload, 0, &range, &resourcedata);
    CopyMemory(resourcedata, &cb0, sizeof(CB0));
    winstate->heap0.constantbuffer0upload->lpVtbl->Unmap(winstate->heap0.constantbuffer0upload, 0, &range);

    // Reuse rb
    D3D12_RESOURCE_BARRIER rb = {0};
    rb.Transition.pResource = winstate->heap0.constantbuffer0;
    rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    rb.Transition.StateBefore = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    rb.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    dxstate->list->lpVtbl->ResourceBarrier(dxstate->list, 1, &rb);

    dxstate->list->lpVtbl->CopyResource(dxstate->list, winstate->heap0.constantbuffer0, winstate->heap0.constantbuffer0upload);

    rb.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    rb.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    dxstate->list->lpVtbl->ResourceBarrier(dxstate->list, 1, &rb);
  }

  // Setup rendering for shadows
  {
    // Bind shader
    D3D12_CPU_DESCRIPTOR_HANDLE shaderdescriptor = DXGetCPUDescriptorHandleForHeapStart(winstate->shaderdescriptorheap);
    dxstate->list->lpVtbl->OMSetRenderTargets(dxstate->list, 0, 0, 0, &shaderdescriptor);

    // Ready depth buffer
    D3D12_RESOURCE_BARRIER rb = {0};
    rb.Transition.pResource = winstate->shaderdepthresource;
    rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    rb.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    rb.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    dxstate->list->lpVtbl->ResourceBarrier(dxstate->list, 1, &rb);

    // Clear buffers
    dxstate->list->lpVtbl->ClearDepthStencilView(dxstate->list, DXGetCPUDescriptorHandleForHeapStart(winstate->shaderdescriptorheap), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, 0);

    // Set pipeline state
    dxstate->list->lpVtbl->SetPipelineState(dxstate->list, winstate->shadershader.pipeline);

    // Set root signature
    dxstate->list->lpVtbl->SetGraphicsRootSignature(dxstate->list, winstate->shadershader.rootsignature);
    
    // Set descriptor heaps
    ID3D12DescriptorHeap *heaps[] = {winstate->heap.heap, winstate->sampler.heap};
    dxstate->list->lpVtbl->SetDescriptorHeaps(dxstate->list, SizeofArray(heaps), heaps);

    // Bind depth buffer for reading
    dxstate->list->lpVtbl->SetGraphicsRootDescriptorTable(dxstate->list, 0, winstate->heap0.constantbuffer0handle.gpuhandle);
    dxstate->list->lpVtbl->SetGraphicsRootDescriptorTable(dxstate->list, 2, winstate->heap0.constantbuffer2handle.gpuhandle);

    // Render scene objects for the shader
    RenderWorld(winstate, FALSE);

    // Ready depth buffer for reading
    rb.Transition.pResource = winstate->shaderdepthresource;
    rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    rb.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    rb.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    dxstate->list->lpVtbl->ResourceBarrier(dxstate->list, 1, &rb);
  }

  // Setup rendering for colour
  {
    // Set render target and depth target
    D3D12_CPU_DESCRIPTOR_HANDLE rtvdescriptor = DXGetCPUDescriptorHandleForHeapStart(dxstate->rendertargetviewdescriptorheap);
    rtvdescriptor.ptr += (UQWORD)frame * dxstate->device->lpVtbl->GetDescriptorHandleIncrementSize(dxstate->device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE dsvdescriptor = DXGetCPUDescriptorHandleForHeapStart(dxstate->depthstencilviewdescriptorheap);
    dxstate->list->lpVtbl->OMSetRenderTargets(dxstate->list, 1, &rtvdescriptor, 0, &dsvdescriptor);

    // Clear buffers
    float colour[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    dxstate->list->lpVtbl->ClearRenderTargetView(dxstate->list, rtvdescriptor, colour, 0, 0);
    dxstate->list->lpVtbl->ClearDepthStencilView(dxstate->list, DXGetCPUDescriptorHandleForHeapStart(dxstate->depthstencilviewdescriptorheap), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, 0);

    // Set pipeline state
    dxstate->list->lpVtbl->SetPipelineState(dxstate->list, winstate->defaultshader.pipeline);

    // Set root signature
    dxstate->list->lpVtbl->SetGraphicsRootSignature(dxstate->list, winstate->defaultshader.rootsignature);

    // Set descriptor heaps
    ID3D12DescriptorHeap *heaps[] = {winstate->heap.heap, winstate->sampler.heap};
    dxstate->list->lpVtbl->SetDescriptorHeaps(dxstate->list, SizeofArray(heaps), heaps);

    // Bind depth buffer for reading (as well as usual buffers)
    dxstate->list->lpVtbl->SetGraphicsRootDescriptorTable(dxstate->list, 0, winstate->heap0.constantbuffer0handle.gpuhandle);
    dxstate->list->lpVtbl->SetGraphicsRootDescriptorTable(dxstate->list, 4, winstate->heap0.constantbuffer2handle.gpuhandle);
    dxstate->list->lpVtbl->SetGraphicsRootDescriptorTable(dxstate->list, 3, winstate->shadertexturehandle.gpuhandle);



    // Render scene objects for the shader
    RenderWorld(winstate, TRUE);
  }
}

void GameRender(WINSTATE *winstate) {
  DX12STATE *dxstate = &winstate->dxstate;

  // Reset command list and allocator
  dxstate->allocator->lpVtbl->Reset(dxstate->allocator);
  dxstate->list->lpVtbl->Reset(dxstate->list, dxstate->allocator, 0);

  // Get current frame
  UDWORD frame = dxstate->swapchain->lpVtbl->GetCurrentBackBufferIndex(dxstate->swapchain);

  // Setup viewport
  D3D12_VIEWPORT viewport = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, 1};
  dxstate->list->lpVtbl->RSSetViewports(dxstate->list, 1, &viewport);

  // Setup scissor
  D3D12_RECT scissor = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
  dxstate->list->lpVtbl->RSSetScissorRects(dxstate->list, 1, &scissor);

  // Transition render target view to a drawing state
  D3D12_RESOURCE_BARRIER rb = {0};
  rb.Transition.pResource = dxstate->rendertargets[frame];
  rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  rb.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
  rb.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
  dxstate->list->lpVtbl->ResourceBarrier(dxstate->list, 1, &rb);

  dxstate->list->lpVtbl->IASetPrimitiveTopology(dxstate->list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  RenderWholeShebang(winstate, frame);

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