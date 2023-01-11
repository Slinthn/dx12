void render_world(struct win64_state *winstate, BOOL coloured) {
  struct dx12_state *dxstate = &winstate->dxstate;

  struct world *world = &winstate->world1;

  // Render all world objects
  for (uint32_t i = 0; i < SIZE_OF_ARRAY(world->objects); i++) {
    struct world_object *obj = &world->objects[i];
    if (!obj->modelindex) {
      continue;
    }

    struct dx12_model *model = &world->models[obj->modelindex];

    D3D12_VERTEX_BUFFER_VIEW views[3] = {
      model->vertexbuffer.view,
      model->texturebuffer.view,
      model->normalbuffer.view
    };

    if (coloured && model->textureindex) {
      dxstate->list->lpVtbl->SetGraphicsRootDescriptorTable(dxstate->list, 2, world->texture[model->textureindex].handle);
    }

    dxstate->list->lpVtbl->SetGraphicsRoot32BitConstant(dxstate->list, 1, i, 0);

    dxstate->list->lpVtbl->IASetVertexBuffers(dxstate->list, 0, SIZE_OF_ARRAY(views), views);
    dxstate->list->lpVtbl->IASetIndexBuffer(dxstate->list, &model->indexbuffer.view);
    dxstate->list->lpVtbl->DrawIndexedInstanced(dxstate->list, model->facecount * 3, 1, 0, 0, 0);
  }
}

void render_shadows(struct win64_state *winstate) {
  struct dx12_state *dxstate = &winstate->dxstate;

  // Bind shader
  D3D12_CPU_DESCRIPTOR_HANDLE shaderdescriptor = dx12_get_cpu_descriptor_handle_for_heap_start(winstate->shadow.descriptorheap);
  dxstate->list->lpVtbl->OMSetRenderTargets(dxstate->list, 0, 0, 0, &shaderdescriptor);

  // Ready depth buffer
  D3D12_RESOURCE_BARRIER rb = {0};
  rb.Transition.pResource = winstate->shadow.depthresource;
  rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  rb.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
  rb.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
  dxstate->list->lpVtbl->ResourceBarrier(dxstate->list, 1, &rb);

  // Clear buffers
  dxstate->list->lpVtbl->ClearDepthStencilView(dxstate->list, dx12_get_cpu_descriptor_handle_for_heap_start(winstate->shadow.descriptorheap), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, 0);

  // Setup viewport
  D3D12_VIEWPORT viewport = {0, 0, SHADOW_WIDTH, SHADOW_HEIGHT, 0, 1};
  dxstate->list->lpVtbl->RSSetViewports(dxstate->list, 1, &viewport);

  // Setup scissor
  D3D12_RECT scissor = {0, 0, SHADOW_WIDTH, SHADOW_HEIGHT};
  dxstate->list->lpVtbl->RSSetScissorRects(dxstate->list, 1, &scissor);

  // Set pipeline state
  dxstate->list->lpVtbl->SetPipelineState(dxstate->list, winstate->shadershader.pipeline);

  // Set root signature
  dxstate->list->lpVtbl->SetGraphicsRootSignature(dxstate->list, winstate->shadershader.rootsignature);
  
  // Set descriptor heaps
  ID3D12DescriptorHeap *heaps[] = {winstate->descriptorheap.heap, winstate->sampler.heap};
  dxstate->list->lpVtbl->SetDescriptorHeaps(dxstate->list, SIZE_OF_ARRAY(heaps), heaps);

  // Bind depth buffer for reading
  dxstate->list->lpVtbl->SetGraphicsRootDescriptorTable(dxstate->list, 0, winstate->constantbuffer0handle.gpuhandle);
  dxstate->list->lpVtbl->SetGraphicsRootDescriptorTable(dxstate->list, 2, winstate->constantbuffer2handle.gpuhandle);

  // Render scene objects for the shader
  render_world(winstate, FALSE);

  // Ready depth buffer for reading
  rb.Transition.pResource = winstate->shadow.depthresource;
  rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  rb.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
  rb.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
  dxstate->list->lpVtbl->ResourceBarrier(dxstate->list, 1, &rb);
}

void render_colour(struct win64_state *winstate, uint32_t frame) {
  struct dx12_state *dxstate = &winstate->dxstate;

  // Set render target and depth target
  D3D12_CPU_DESCRIPTOR_HANDLE rtvdescriptor = dx12_get_cpu_descriptor_handle_for_heap_start(dxstate->rendertargetviewdescriptorheap);
  rtvdescriptor.ptr += (uint64_t)frame * dxstate->device->lpVtbl->GetDescriptorHandleIncrementSize(dxstate->device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  D3D12_CPU_DESCRIPTOR_HANDLE dsvdescriptor = dx12_get_cpu_descriptor_handle_for_heap_start(dxstate->depthstencilviewdescriptorheap);
  dxstate->list->lpVtbl->OMSetRenderTargets(dxstate->list, 1, &rtvdescriptor, 0, &dsvdescriptor);

  // Clear buffers
  float colour[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  dxstate->list->lpVtbl->ClearRenderTargetView(dxstate->list, rtvdescriptor, colour, 0, 0);
  dxstate->list->lpVtbl->ClearDepthStencilView(dxstate->list, dx12_get_cpu_descriptor_handle_for_heap_start(dxstate->depthstencilviewdescriptorheap), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, 0);

  // Set pipeline state
  dxstate->list->lpVtbl->SetPipelineState(dxstate->list, winstate->defaultshader.pipeline);

  // Set root signature
  dxstate->list->lpVtbl->SetGraphicsRootSignature(dxstate->list, winstate->defaultshader.rootsignature);

  // Setup viewport
  D3D12_VIEWPORT viewport = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, 1};
  dxstate->list->lpVtbl->RSSetViewports(dxstate->list, 1, &viewport);

  // Setup scissor
  D3D12_RECT scissor = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
  dxstate->list->lpVtbl->RSSetScissorRects(dxstate->list, 1, &scissor);

  // Set descriptor heaps
  ID3D12DescriptorHeap *heaps[] = {winstate->descriptorheap.heap, winstate->sampler.heap};
  dxstate->list->lpVtbl->SetDescriptorHeaps(dxstate->list, SIZE_OF_ARRAY(heaps), heaps);

  // Bind depth buffer for reading (as well as usual buffers)
  dxstate->list->lpVtbl->SetGraphicsRootDescriptorTable(dxstate->list, 0, winstate->constantbuffer0handle.gpuhandle);
  dxstate->list->lpVtbl->SetGraphicsRootDescriptorTable(dxstate->list, 4, winstate->constantbuffer2handle.gpuhandle);
  dxstate->list->lpVtbl->SetGraphicsRootDescriptorTable(dxstate->list, 3, winstate->shadow.texturehandle.gpuhandle);

  // Render scene objects for the shader
  render_world(winstate, TRUE);
}

void render_setup_constant_buffers(struct win64_state *winstate) {
  struct dx12_state *dxstate = &winstate->dxstate;

  // Setup constant_buffer2  
  {
    struct constant_buffer2 cb2 = {0};

    // TODO
    vec3_copy(&winstate->player.transform.scale, (struct vector3){1, 1, 1});
    vec3_copy(&winstate->sun.scale, (struct vector3){1, 1, 1});
    vec3_copy(&winstate->sun.position, (struct vector3){0, 50, 0});
    vec3_copy(&winstate->sun.rotation, (struct vector3){DEGREESTORADIANS(90), 0, 0});
    //vec3_copy(&winstate->player.transform.position, (struct vector3){0, 2, 0});

    mat4_perspective(&cb2.cameraperspective, WINDOW_HEIGHT / (float)WINDOW_WIDTH, DEGREESTORADIANS(100.0f), 0.1f, 100.0f);
    mat4_inverse_transform(&cb2.camera, winstate->player.transform);
    mat4_orthographic(&cb2.sunperspective, -100, 100, -100, 100, 0.1f, 100.0f);
    mat4_inverse_transform(&cb2.suncamera, winstate->sun);


    dx12_update_buffer(dxstate, winstate->constantbuffer2, winstate->constantbuffer2upload, &cb2);
  }

  // Setup constant_buffer0
  {
    struct world *world = &winstate->world1;

    struct constant_buffer0 cb0 = {0};

    for (uint32_t i = 0; i < SIZE_OF_ARRAY(world->objects); i++) {
      struct world_object *obj = &world->objects[i];
      // Check if object is valid and should be rendered
      if (!obj->modelindex) {
        continue;
      }

      struct dx12_model *model = &world->models[obj->modelindex];

      // Update constant buffer
      mat4_transform(&cb0.data[i].transform, obj->transform);
      vec4_copy(&cb0.data[i].colour, model->colour);
    }

    dx12_update_buffer(dxstate, winstate->constantbuffer0, winstate->constantbuffer0upload, &cb0);
  }
}

void game_render(struct win64_state *winstate) {
  struct dx12_state *dxstate = &winstate->dxstate;

  // Reset command list and allocator
  dxstate->allocator->lpVtbl->Reset(dxstate->allocator);
  dxstate->list->lpVtbl->Reset(dxstate->list, dxstate->allocator, 0);
  
  // Get current frame
  uint32_t frame = dxstate->swapchain->lpVtbl->GetCurrentBackBufferIndex(dxstate->swapchain);

  // Transition render target view to a drawing state
  D3D12_RESOURCE_BARRIER rb = {0};
  rb.Transition.pResource = dxstate->rendertargets[frame];
  rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  rb.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
  rb.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
  dxstate->list->lpVtbl->ResourceBarrier(dxstate->list, 1, &rb);

  dxstate->list->lpVtbl->IASetPrimitiveTopology(dxstate->list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  // Setup constant buffers
  render_setup_constant_buffers(winstate);

  // Render shadows
  render_shadows(winstate);

  // Render colour
  render_colour(winstate, frame);

  // Transition render target view to a present state
  rb.Transition.pResource = dxstate->rendertargets[frame];
  rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  rb.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
  rb.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
  dxstate->list->lpVtbl->ResourceBarrier(dxstate->list, 1, &rb);

  // Close, execute command list and wait for GPU
  dxstate->list->lpVtbl->Close(dxstate->list);
  dxstate->queue->lpVtbl->ExecuteCommandLists(dxstate->queue, 1, (ID3D12CommandList **)&dxstate->list);
  dx12_wait_for_fence(dxstate);

  // Present frame to the screen
  dxstate->swapchain->lpVtbl->Present(dxstate->swapchain, 1, 0);
}
