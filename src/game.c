#include "renderer/renderer.c"

#include "physics.c"

void init_heap0(win64_state *winstate) {
  dx12_state *dxstate = &winstate->dxstate;

  // Calculate the starting byte of each asset in the heap, and hence the total size required
  u64 constantbuffer0offset = 0;
  u64 constantbuffer2offset = ALIGN_UP(constantbuffer0offset + sizeof(constant_buffer0), 1024 * 64);
  u64 totalsize = ALIGN_UP(constantbuffer2offset + sizeof(constant_buffer2), 1024 * 64);

  // Create normal descriptorheap and upload heap
  winstate->heap0.heap = dx12_create_heap(dxstate, totalsize, D3D12_HEAP_TYPE_DEFAULT);
  winstate->heap0.uploadheap = dx12_create_heap(dxstate, totalsize, D3D12_HEAP_TYPE_UPLOAD);
  
  // Create constant buffers
  winstate->heap0.constantbuffer0 = dx12_create_buffer_resource(dxstate, ALIGN_UP(sizeof(constant_buffer0), 256), winstate->heap0.heap, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, constantbuffer0offset);
  winstate->heap0.constantbuffer0upload = dx12_create_buffer_resource(dxstate, ALIGN_UP(sizeof(constant_buffer0), 256), winstate->heap0.uploadheap, D3D12_RESOURCE_STATE_GENERIC_READ, constantbuffer0offset);

  winstate->heap0.constantbuffer2 = dx12_create_buffer_resource(dxstate, ALIGN_UP(sizeof(constant_buffer2), 256), winstate->heap0.heap, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, constantbuffer2offset);
  winstate->heap0.constantbuffer2upload = dx12_create_buffer_resource(dxstate, ALIGN_UP(sizeof(constant_buffer2), 256), winstate->heap0.uploadheap, D3D12_RESOURCE_STATE_GENERIC_READ, constantbuffer2offset);

  // Create the constant buffer view using the descriptor heap
  D3D12_CONSTANT_BUFFER_VIEW_DESC constantbufferviewdesc = {0};
  constantbufferviewdesc.BufferLocation = winstate->heap0.constantbuffer0->lpVtbl->GetGPUVirtualAddress(winstate->heap0.constantbuffer0);
  constantbufferviewdesc.SizeInBytes = ALIGN_UP(sizeof(constant_buffer0), 256);

  winstate->heap0.constantbuffer0handle = dx12_get_next_unused_handle(dxstate, &winstate->descriptorheap);
  dxstate->device->lpVtbl->CreateConstantBufferView(dxstate->device, &constantbufferviewdesc, winstate->heap0.constantbuffer0handle.cpuhandle);

  // Create another constant buffer view using the descriptor heap (reuse constantbufferviewdesc)
  constantbufferviewdesc.BufferLocation = winstate->heap0.constantbuffer2->lpVtbl->GetGPUVirtualAddress(winstate->heap0.constantbuffer2);
  constantbufferviewdesc.SizeInBytes = ALIGN_UP(sizeof(constant_buffer2), 256);

  winstate->heap0.constantbuffer2handle = dx12_get_next_unused_handle(dxstate, &winstate->descriptorheap);
  dxstate->device->lpVtbl->CreateConstantBufferView(dxstate->device, &constantbufferviewdesc, winstate->heap0.constantbuffer2handle.cpuhandle);
}

void game_init(win64_state *winstate) {
  dx12_state *dxstate = &winstate->dxstate;

  // Create vertex shader and pixel shader
  D3D12_INPUT_ELEMENT_DESC ieds[] =
  {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
  };

  D3D12_INPUT_LAYOUT_DESC inputlayoutdesc = {0};
  inputlayoutdesc.pInputElementDescs = ieds;
  inputlayoutdesc.NumElements = SIZE_OF_ARRAY(ieds);

  {
    win64_resource vertex = win64_load_resource(RESOURCE_DEFAULT_VERTEX, RESOURCE_TYPE_HLSL);
    win64_resource pixel = win64_load_resource(RESOURCE_DEFAULT_PIXEL, RESOURCE_TYPE_HLSL);
    winstate->defaultshader = dx12_create_shader(dxstate, vertex.data, vertex.size, pixel.data, pixel.size, inputlayoutdesc);
  }

  // Create shader vertex shader and pixel shader (with same input layout descriptor)
  {
    win64_resource vertex = win64_load_resource(RESOURCE_SHADER_VERTEX, RESOURCE_TYPE_HLSL);
    win64_resource pixel = win64_load_resource(RESOURCE_SHADER_PIXEL, RESOURCE_TYPE_HLSL);
    winstate->shadershader = dx12_create_shader(dxstate, vertex.data, vertex.size, pixel.data, pixel.size, inputlayoutdesc);
  }

  // Create sampler
  winstate->sampler = dx12_create_sampler(dxstate);

  // Reset the command list and allocator in order to upload resources to heap0
  dxstate->allocator->lpVtbl->Reset(dxstate->allocator);
  dxstate->list->lpVtbl->Reset(dxstate->list, dxstate->allocator, winstate->defaultshader.pipeline);

  // Create descriptor heaps for both heap0 and heap1
  winstate->descriptorheap = dx12_create_descriptor_heap(dxstate, 128, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

  // Initialise heap
  init_heap0(winstate);

  // Initialise world
  win64_resource r = win64_load_resource(RESOURCE_WORLD0, RESOURCE_TYPE_WORLD);
  // TODO I think there's a problem with loading multiple worlds. The descriptor heaps suddenly don't line up (which is ok in the gltf_load call) but when accessing the descriptors in the SetRootDescriptorTables function, it's wrong
  // To be honest, i'm not sure. but i'll put this comment here just in case i forget in the future and something messes up and i don't know why

  winstate->world1 = gltf_load(winstate, r.data, r.size, &winstate->descriptorheap);

  // Close list, execute commands and wait
  dxstate->list->lpVtbl->Close(dxstate->list);
  dxstate->queue->lpVtbl->ExecuteCommandLists(dxstate->queue, 1, (ID3D12CommandList **)&dxstate->list);
  dx12_wait_for_fence(dxstate);

  // TODO temporary, change colours
  for (u32 i = 0; i < SIZE_OF_ARRAY(winstate->world1.models); i++) {
    vec4_copy(&winstate->world1.models[i].colour, (vec4){0, 1, 1, 1});
  }

  vec4_copy(&winstate->world1.models[2].colour, (vec4){0, 0, 0, 0});
  vec4_copy(&winstate->world1.models[3].colour, (vec4){0, 0, 0, 0});
  vec4_copy(&winstate->world1.models[4].colour, (vec4){0, 0, 0, 0});

  // TODO temp shader buffer
  winstate->shadow = dx12_create_shadow_buffer(dxstate, &winstate->descriptorheap);
}

void game_update(win64_state *winstate) {
  player *player = &winstate->player;
  transformation *transform = &player->transform;

  // Parse look movements
  vec3 look = {winstate->controls.look[1],  winstate->controls.look[0], 0};
  float lookmag = vec3_magnitude(look);
  if (lookmag > 1.0f) {
    vec3_normalise(&look);
  } else if (lookmag < 0.1f) {
    vec3_identity(&look);
  }

  vec3_mul(&look, (vec3){0.1f, 0.1f, 0});
  vec3_add(&transform->rotation, look);

  // Parse move movements
  vec2 move;
  vec2_copy(&move, winstate->controls.move);
  float movemag = VECMagnitude2f(move);
  if (movemag > 1.0f) {
    vec2_normalise(&move);
  } else if (movemag < 0.1f) {
    vec2_identity(&move);
  }

  vec2_mul(&move, (vec2){0.1f, 0.1f});

  float rotcos = cosf(winstate->player.transform.rotation[1]);
  float rotsin = sinf(winstate->player.transform.rotation[1]);

  vec3 movedir = {0};
  movedir[0] = move[0] * rotcos - move[1] * rotsin;
  movedir[2] = -move[0] * rotsin - move[1] * rotcos;

  vec3_add(&winstate->player.transform.position, movedir);

  winstate->player.velocity[1] = 0;

  if (winstate->controls.actions & ACTION_ASCEND) {
    winstate->player.velocity[1] = 0.5f;
  }

  if (winstate->controls.actions & ACTION_DESCEND) {
    winstate->player.velocity[1] = -0.5f;
  }

  vec3_add(&transform->position, winstate->player.velocity);

  if (winstate->player.transform.position[1] < 0) {
    winstate->player.transform.position[1] = 0;
  }

  // Gravity
  //winstate->player.velocity[1] -= 0.05f;
  
  world_physics(winstate);

  game_render(winstate);
}
