#include "renderer.c"

void InitialiseHeap0(WINSTATE *winstate) {
  DX12STATE *dxstate = &winstate->dxstate;

  // Calculate the starting byte of each asset in the heap, and hence the total size required
  UQWORD constantbuffer0offset = 0;
  UQWORD constantbuffer2offset = AlignUp(constantbuffer0offset + sizeof(CB0), 1024 * 64);
  UQWORD totalsize = AlignUp(constantbuffer2offset + sizeof(CB2), 1024 * 64);

  // Create normal descriptorheap and upload heap
  winstate->heap0.heap = DXCreateHeap(dxstate, totalsize, D3D12_HEAP_TYPE_DEFAULT);
  winstate->heap0.uploadheap = DXCreateHeap(dxstate, totalsize, D3D12_HEAP_TYPE_UPLOAD);
  
  // Create a constant buffers
  winstate->heap0.constantbuffer0 = DXCreateBufferResource(dxstate, AlignUp(sizeof(CB0), 256), winstate->heap0.heap, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, constantbuffer0offset);
  winstate->heap0.constantbuffer0upload = DXCreateBufferResource(dxstate, AlignUp(sizeof(CB0), 256), winstate->heap0.uploadheap, D3D12_RESOURCE_STATE_GENERIC_READ, constantbuffer0offset);

  winstate->heap0.constantbuffer2 = DXCreateBufferResource(dxstate, AlignUp(sizeof(CB2), 256), winstate->heap0.heap, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, constantbuffer2offset);
  winstate->heap0.constantbuffer2upload = DXCreateBufferResource(dxstate, AlignUp(sizeof(CB2), 256), winstate->heap0.uploadheap, D3D12_RESOURCE_STATE_GENERIC_READ, constantbuffer2offset);

  // Create the constant buffer view using the descriptor heap
  D3D12_CONSTANT_BUFFER_VIEW_DESC constantbufferviewdesc = {0};
  constantbufferviewdesc.BufferLocation = winstate->heap0.constantbuffer0->lpVtbl->GetGPUVirtualAddress(winstate->heap0.constantbuffer0);
  constantbufferviewdesc.SizeInBytes = AlignUp(sizeof(CB0), 256);

  winstate->heap0.constantbuffer0handle = DXGetNextUnusedHandle(dxstate, &winstate->heap);
  dxstate->device->lpVtbl->CreateConstantBufferView(dxstate->device, &constantbufferviewdesc, winstate->heap0.constantbuffer0handle.cpuhandle);

  // Create another constant buffer view using the descriptor heap (reuse constantbufferviewdesc)
  constantbufferviewdesc.BufferLocation = winstate->heap0.constantbuffer2->lpVtbl->GetGPUVirtualAddress(winstate->heap0.constantbuffer2);
  constantbufferviewdesc.SizeInBytes = AlignUp(sizeof(CB2), 256);

  winstate->heap0.constantbuffer2handle = DXGetNextUnusedHandle(dxstate, &winstate->heap);
  dxstate->device->lpVtbl->CreateConstantBufferView(dxstate->device, &constantbufferviewdesc, winstate->heap0.constantbuffer2handle.cpuhandle);
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

  {
    WINRESOURCE vertex = WINLoadResource(RESOURCE_DEFAULT_VERTEX, RESOURCE_TYPE_HLSL);
    WINRESOURCE pixel = WINLoadResource(RESOURCE_DEFAULT_PIXEL, RESOURCE_TYPE_HLSL);
    winstate->defaultshader = DXCreateShader(dxstate, vertex.data, vertex.size, pixel.data, pixel.size, inputlayoutdesc);
  }

  // Create shader vertex shader and pixel shader (with same input layout descriptor)
  {
    WINRESOURCE vertex = WINLoadResource(RESOURCE_SHADER_VERTEX, RESOURCE_TYPE_HLSL);
    WINRESOURCE pixel = WINLoadResource(RESOURCE_SHADER_PIXEL, RESOURCE_TYPE_HLSL);
    winstate->shadershader = DXCreateShader(dxstate, vertex.data, vertex.size, pixel.data, pixel.size, inputlayoutdesc);
  }

  // Create sampler
  winstate->sampler = DXCreateSampler(dxstate);

  // Reset the command list and allocator in order to upload resources to heap0
  dxstate->allocator->lpVtbl->Reset(dxstate->allocator);
  dxstate->list->lpVtbl->Reset(dxstate->list, dxstate->allocator, winstate->defaultshader.pipeline);

  // Create descriptor heaps for both heap0 and heap1
  winstate->heap = DXCreateDescriptorHeap(dxstate, 128, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

  // Initialise heap
  InitialiseHeap0(winstate);

  // Initialise world
  WINRESOURCE r = WINLoadResource(RESOURCE_WORLD0, RESOURCE_TYPE_WORLD);
  // TODO I think there's a problem with loading multiple worlds. The descriptor heaps suddenly don't line up (which is ok in the LoadGLTF call) but when accessing the descriptors in the SetRootDescriptorTables function, it's wrong
  // To be honest, i'm not sure. but i'll put this comment here just in case i forget in the future and something messes up and i don't know why

  winstate->world1 = LoadGLTF(winstate, r.data, r.size, &winstate->heap);

  // Close list, execute commands and wait
  dxstate->list->lpVtbl->Close(dxstate->list);
  dxstate->queue->lpVtbl->ExecuteCommandLists(dxstate->queue, 1, (ID3D12CommandList **)&dxstate->list);
  DXWaitForFence(dxstate);

  // TODO temporary, change colours
  for (UDWORD i = 0; i < SizeofArray(winstate->world1.models); i++) {
    VECCopy4f(&winstate->world1.models[i].colour, (VECTOR4F){0, 1, 1, 1});
  }

  VECCopy4f(&winstate->world1.models[2].colour, (VECTOR4F){0, 0, 0, 0});
  VECCopy4f(&winstate->world1.models[3].colour, (VECTOR4F){0, 0, 0, 0});
  VECCopy4f(&winstate->world1.models[4].colour, (VECTOR4F){0, 0, 0, 0});

  // TODO temp shader buffer
  D3D12_DESCRIPTOR_HEAP_DESC descriptorheapdesc = {0};
  descriptorheapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
  descriptorheapdesc.NumDescriptors = 1;

  dxstate->device->lpVtbl->CreateDescriptorHeap(dxstate->device, &descriptorheapdesc, &IID_ID3D12DescriptorHeap, &winstate->shaderdescriptorheap);

  {
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
    resourcedesc.Format = DXGI_FORMAT_R32_TYPELESS;
    resourcedesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearvalue = {0};
    clearvalue.Format = DXGI_FORMAT_D32_FLOAT;
    clearvalue.DepthStencil.Depth = 1.0f;
  
    dxstate->device->lpVtbl->CreateCommittedResource(dxstate->device, &heapproperties, 0, &resourcedesc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearvalue, &IID_ID3D12Resource, &winstate->shaderdepthresource); // TODO make placed resource?

    // Retrieve depth stencil view
    D3D12_DEPTH_STENCIL_VIEW_DESC depthstencilviewdesc = {0};
    depthstencilviewdesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthstencilviewdesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

    dxstate->device->lpVtbl->CreateDepthStencilView(dxstate->device, winstate->shaderdepthresource, &depthstencilviewdesc, DXGetCPUDescriptorHandleForHeapStart(winstate->shaderdescriptorheap));

    D3D12_SHADER_RESOURCE_VIEW_DESC shaderresourceviewdesc = {0};
    shaderresourceviewdesc.Format = DXGI_FORMAT_R32_FLOAT;
    shaderresourceviewdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    shaderresourceviewdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    shaderresourceviewdesc.Texture2D.MipLevels = 1;

    winstate->shadertexturehandle = DXGetNextUnusedHandle(dxstate, &winstate->heap);

    dxstate->device->lpVtbl->CreateShaderResourceView(dxstate->device, winstate->shaderdepthresource, &shaderresourceviewdesc, winstate->shadertexturehandle.cpuhandle);
  }
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