#include "renderer.c"

void InitialiseHeap0(WINSTATE *winstate) {
  DX12STATE *dxstate = &winstate->dxstate;

  // Calculate the starting byte of each asset in the heap, and hence the total size required
  U64 constantbuffer0offset = 0;
  U64 constantbuffer2offset = AlignUp(constantbuffer0offset + sizeof(CB0), 1024 * 64);
  U64 totalsize = AlignUp(constantbuffer2offset + sizeof(CB2), 1024 * 64);

  // Create normal descriptorheap and upload heap
  winstate->heap0.heap = DXCreateHeap(dxstate, totalsize, D3D12_HEAP_TYPE_DEFAULT);
  winstate->heap0.uploadheap = DXCreateHeap(dxstate, totalsize, D3D12_HEAP_TYPE_UPLOAD);
  
  // Create constant buffers
  winstate->heap0.constantbuffer0 = DXCreateBufferResource(dxstate, AlignUp(sizeof(CB0), 256), winstate->heap0.heap, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, constantbuffer0offset);
  winstate->heap0.constantbuffer0upload = DXCreateBufferResource(dxstate, AlignUp(sizeof(CB0), 256), winstate->heap0.uploadheap, D3D12_RESOURCE_STATE_GENERIC_READ, constantbuffer0offset);

  winstate->heap0.constantbuffer2 = DXCreateBufferResource(dxstate, AlignUp(sizeof(CB2), 256), winstate->heap0.heap, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, constantbuffer2offset);
  winstate->heap0.constantbuffer2upload = DXCreateBufferResource(dxstate, AlignUp(sizeof(CB2), 256), winstate->heap0.uploadheap, D3D12_RESOURCE_STATE_GENERIC_READ, constantbuffer2offset);

  // Create the constant buffer view using the descriptor heap
  D3D12_CONSTANT_BUFFER_VIEW_DESC constantbufferviewdesc = {0};
  constantbufferviewdesc.BufferLocation = winstate->heap0.constantbuffer0->lpVtbl->GetGPUVirtualAddress(winstate->heap0.constantbuffer0);
  constantbufferviewdesc.SizeInBytes = AlignUp(sizeof(CB0), 256);

  winstate->heap0.constantbuffer0handle = DXGetNextUnusedHandle(dxstate, &winstate->descriptorheap);
  dxstate->device->lpVtbl->CreateConstantBufferView(dxstate->device, &constantbufferviewdesc, winstate->heap0.constantbuffer0handle.cpuhandle);

  // Create another constant buffer view using the descriptor heap (reuse constantbufferviewdesc)
  constantbufferviewdesc.BufferLocation = winstate->heap0.constantbuffer2->lpVtbl->GetGPUVirtualAddress(winstate->heap0.constantbuffer2);
  constantbufferviewdesc.SizeInBytes = AlignUp(sizeof(CB2), 256);

  winstate->heap0.constantbuffer2handle = DXGetNextUnusedHandle(dxstate, &winstate->descriptorheap);
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
  winstate->descriptorheap = DXCreateDescriptorHeap(dxstate, 128, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

  // Initialise heap
  InitialiseHeap0(winstate);

  // Initialise world
  WINRESOURCE r = WINLoadResource(RESOURCE_WORLD0, RESOURCE_TYPE_WORLD);
  // TODO I think there's a problem with loading multiple worlds. The descriptor heaps suddenly don't line up (which is ok in the LoadGLTF call) but when accessing the descriptors in the SetRootDescriptorTables function, it's wrong
  // To be honest, i'm not sure. but i'll put this comment here just in case i forget in the future and something messes up and i don't know why

  winstate->world1 = LoadGLTF(winstate, r.data, r.size, &winstate->descriptorheap);

  // Close list, execute commands and wait
  dxstate->list->lpVtbl->Close(dxstate->list);
  dxstate->queue->lpVtbl->ExecuteCommandLists(dxstate->queue, 1, (ID3D12CommandList **)&dxstate->list);
  DXWaitForFence(dxstate);

  // TODO temporary, change colours
  for (U32 i = 0; i < SizeofArray(winstate->world1.models); i++) {
    VECCopy4f(&winstate->world1.models[i].colour, (VECTOR4F){0, 1, 1, 1});
  }

  VECCopy4f(&winstate->world1.models[2].colour, (VECTOR4F){0, 0, 0, 0});
  VECCopy4f(&winstate->world1.models[3].colour, (VECTOR4F){0, 0, 0, 0});
  VECCopy4f(&winstate->world1.models[4].colour, (VECTOR4F){0, 0, 0, 0});

  // TODO temp shader buffer
  winstate->shadow = DXCreateShadows(dxstate, &winstate->descriptorheap);
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

  winstate->player.velocity[1] = 0;

  if (winstate->controls.actions & ACTION_ASCEND) {
    winstate->player.velocity[1] = 0.5f;
  }

  if (winstate->controls.actions & ACTION_DESCEND) {
    winstate->player.velocity[1] = -0.5f;
  }

  VECAdd3f(&transform->position, winstate->player.velocity);

  if (winstate->player.transform.position[1] < 0) {
    winstate->player.transform.position[1] = 0;
  }

  // Gravity
  //winstate->player.velocity[1] -= 0.05f;
  
  GameRender(winstate);
}