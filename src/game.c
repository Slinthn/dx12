#include "renderer/renderer.c"

#include "physics.c"

void game_create_constant_buffers(struct win64_state *winstate) {
  struct dx12_state *dxstate = &winstate->dxstate;

  // Calculate the starting byte of each asset in the heap,
  //   and hence the total size required
  uint64_t constantbuffer0offset = 0;

  uint64_t constantbuffer2offset = ALIGN_UP(constantbuffer0offset
    + sizeof(struct constant_buffer0), 1024 * 64);

  uint64_t totalsize = ALIGN_UP(constantbuffer2offset
    + sizeof(struct constant_buffer2), 1024 * 64);

  // Create normal heap and upload heap
  winstate->heap = dx12_create_heap(dxstate, totalsize,
    D3D12_HEAP_TYPE_DEFAULT);

  winstate->uploadheap = dx12_create_heap(dxstate, totalsize,
    D3D12_HEAP_TYPE_UPLOAD);

  // Create constant buffers
  winstate->constantbuffer0 = dx12_create_buffer_resource(dxstate,
    ALIGN_UP(sizeof(struct constant_buffer0), 256), winstate->heap,
    D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, constantbuffer0offset);

  winstate->constantbuffer0upload = dx12_create_buffer_resource(dxstate,
    ALIGN_UP(sizeof(struct constant_buffer0), 256), winstate->uploadheap,
    D3D12_RESOURCE_STATE_GENERIC_READ, constantbuffer0offset);

  winstate->constantbuffer2 = dx12_create_buffer_resource(dxstate,
    ALIGN_UP(sizeof(struct constant_buffer2), 256), winstate->heap,
    D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, constantbuffer2offset);

  winstate->constantbuffer2upload = dx12_create_buffer_resource(dxstate,
    ALIGN_UP(sizeof(struct constant_buffer2), 256), winstate->uploadheap,
    D3D12_RESOURCE_STATE_GENERIC_READ, constantbuffer2offset);

  // Create the constant buffer view using the descriptor heap
  winstate->constantbuffer0handle = dx12_create_constant_buffer_view(dxstate,
    &winstate->descriptorheap, winstate->constantbuffer0,
    sizeof(struct constant_buffer0));

  winstate->constantbuffer2handle = dx12_create_constant_buffer_view(dxstate,
    &winstate->descriptorheap, winstate->constantbuffer2,
    sizeof(struct constant_buffer2));
}

void game_create_shaders(struct win64_state *winstate) {
  struct dx12_state *dxstate = &winstate->dxstate;

  // Create vertex shader and pixel shader
  D3D12_INPUT_ELEMENT_DESC ieds[] =
  {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
      0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},

    {"TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT,
      1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},

    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,
      2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
  };

  D3D12_INPUT_LAYOUT_DESC inputlayoutdesc = {0};
  inputlayoutdesc.pInputElementDescs = ieds;
  inputlayoutdesc.NumElements = SIZE_OF_ARRAY(ieds);

  {
    struct win64_resource vertex = win64_load_resource(RESOURCE_DEFAULT_VERTEX,
      RESOURCE_TYPE_HLSL);

    struct win64_resource pixel = win64_load_resource(RESOURCE_DEFAULT_PIXEL,
      RESOURCE_TYPE_HLSL);

    winstate->defaultshader = dx12_create_shader(dxstate, vertex.data,
      vertex.size, pixel.data, pixel.size, inputlayoutdesc);
  }

  // Create shader vertex shader and pixel shader
  //   (with same input layout descriptor)
  {
    struct win64_resource vertex = win64_load_resource(RESOURCE_SHADER_VERTEX,
      RESOURCE_TYPE_HLSL);

    struct win64_resource pixel = win64_load_resource(RESOURCE_SHADER_PIXEL,
      RESOURCE_TYPE_HLSL);

    winstate->shadershader = dx12_create_shader(dxstate, vertex.data,
      vertex.size, pixel.data, pixel.size, inputlayoutdesc);
  }
}

void game_create_world(struct win64_state *winstate) {
  // Initialise world
  struct win64_resource r = win64_load_resource(RESOURCE_WORLD0, RESOURCE_TYPE_WORLD);
  // TODO I think there's a problem with loading multiple worlds.
  //   The descriptor heaps suddenly don't line up (which is ok in the
  //   gltf_load call), but when accessing the descriptors in the
  //   SetRootDescriptorTables function, it's wrong.
  //   To be honest, i'm not sure, but i'll put this comment here just in
  //    case i forget in the future and something messes up
  //    and i don't know why

  winstate->world1 = gltf_load(winstate, r.data, r.size,
    &winstate->descriptorheap);

  // TODO temporary, change colours
  for (uint32_t i = 0; i < SIZE_OF_ARRAY(winstate->world1.models); i++) {
    vec4_copy(&winstate->world1.models[i].colour, (struct vector4){0, 1, 1, 1});
  }

  vec4_copy(&winstate->world1.models[2].colour, (struct vector4){0, 0, 0, 0});
  vec4_copy(&winstate->world1.models[3].colour, (struct vector4){0, 0, 0, 0});
  vec4_copy(&winstate->world1.models[4].colour, (struct vector4){0, 0, 0, 0});
}

void game_init(struct win64_state *winstate) {
  struct dx12_state *dxstate = &winstate->dxstate;

  game_create_shaders(winstate);

  // Create sampler
  winstate->sampler = dx12_create_sampler(dxstate);

  // Reset the command list and allocator in order to upload resources to heap0
  dxstate->allocator->lpVtbl->Reset(dxstate->allocator);
  dxstate->list->lpVtbl->Reset(dxstate->list, dxstate->allocator,
    winstate->defaultshader.pipeline);

  // Create descriptor heaps for both heap0 and heap1
  winstate->descriptorheap = dx12_create_descriptor_heap(dxstate, 128,
    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
    D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

  game_create_world(winstate);

  // Close list, execute commands and wait
  dxstate->list->lpVtbl->Close(dxstate->list);
  dxstate->queue->lpVtbl->ExecuteCommandLists(dxstate->queue, 1,
    (ID3D12CommandList **)&dxstate->list);

  dx12_wait_for_fence(dxstate);

  // Initialise heap
  game_create_constant_buffers(winstate);

  // Create shadow buffer
  winstate->shadow = dx12_create_shadow_buffer(dxstate,
    &winstate->descriptorheap);
}

void game_update(struct win64_state *winstate) {
  struct player *player = &winstate->player;
  struct transformation *transform = &player->transform;

  // Parse look movements
  struct vector3 look = {winstate->controls.look.y,
    winstate->controls.look.x, 0};

  float lookmag = vec3_magnitude(look);
  if (lookmag > 1.0f) {
    vec3_normalise(&look);
  } else if (lookmag < 0.1f) {
    vec3_identity(&look);
  }

  vec3_mul(&look, (struct vector3){0.1f, 0.1f, 0});
  vec3_add(&transform->rotation, look);

  // Parse move movements
  struct vector2 move;
  vec2_copy(&move, winstate->controls.move);
  float movemag = vec2_magnitude(move);
  if (movemag > 1.0f) {
    vec2_normalise(&move);
  } else if (movemag < 0.1f) {
    vec2_identity(&move);
  }

  vec2_mul(&move, (struct vector2){0.1f, 0.1f});

  float rotcos = cosf(winstate->player.transform.rotation.y);
  float rotsin = sinf(winstate->player.transform.rotation.y);

  struct vector3 movedir = {0};
  movedir.x = move.x * rotcos - move.y * rotsin;
  movedir.z = -move.x * rotsin - move.y * rotcos;

  vec3_add(&winstate->player.transform.position, movedir);

  winstate->player.velocity.y = 0;

  if (winstate->controls.actions & ACTION_ASCEND) {
    winstate->player.velocity.y = 0.5f;
  }

  if (winstate->controls.actions & ACTION_DESCEND) {
    winstate->player.velocity.y = -0.5f;
  }

  vec3_add(&transform->position, winstate->player.velocity);

  if (winstate->player.transform.position.y < 0) {
    winstate->player.transform.position.y = 0;
  }

  // Gravity
  //winstate->player.velocity[1] -= 0.05f;
  
  world_physics(winstate);

  game_render(winstate);
}
