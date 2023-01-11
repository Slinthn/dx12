// TODO this file will be completely rewritten. i hate it

struct win64_resource win64_load_resource(uint32_t name, uint32_t type) {
  HRSRC src = FindResource(0, MAKEINTRESOURCE(name), MAKEINTRESOURCE(type));
  HGLOBAL global = LoadResource(0, src);
  void *data = LockResource(global);
  uint32_t size = SizeofResource(0, src);

  struct win64_resource res = {0};
  res.src = src;
  res.global = global;
  res.data = data;
  res.size = size;
  return res;
}

struct world gltf_load(struct win64_state *winstate, void *gltfdata, uint64_t sizeinbytes, struct dx12_descriptor_heap *heap) {
  struct world ret = {0};

  uint64_t indexoffset = 1; // Make the first entry in each array (objects, textures, etc) a null entry

  struct dx12_state *dxstate = &winstate->dxstate;

  cgltf_options options = {0};
  cgltf_data* bufdata;
  cgltf_parse(&options, gltfdata, sizeinbytes, &bufdata);
  cgltf_load_buffers(&options, bufdata, 0);

  uint64_t heaptotalsize = 0; // This variable is to store the size of the WHOLE allocated heap
                         // (to be used later when allocating memory for the heap)
#if 1
  for (uint32_t i = 0; i < bufdata->meshes_count; i++) {
    // Get heap required size for models
    cgltf_buffer_view *indexbuffer = bufdata->meshes[i].primitives->indices->buffer_view;
    cgltf_buffer_view *vertexbuffer = bufdata->meshes[i].primitives->attributes[0].data->buffer_view;
    cgltf_buffer_view *texturebuffer = bufdata->meshes[i].primitives->attributes[2].data->buffer_view;
    cgltf_buffer_view *normalbuffer = bufdata->meshes[i].primitives->attributes[1].data->buffer_view;

    uint64_t vertexbufferoffset = 0;
    uint64_t texturebufferoffset = ALIGN_UP(vertexbufferoffset + vertexbuffer->size, 1024 * 64);
    uint64_t normalbufferoffset = ALIGN_UP(texturebufferoffset + texturebuffer->size, 1024 * 64);
    uint64_t indexbufferoffset = ALIGN_UP(normalbufferoffset + normalbuffer->size, 1024 * 64);
    heaptotalsize += ALIGN_UP(indexbufferoffset + indexbuffer->size, 1024 * 64);
  }
#endif // 0


  for (uint32_t i = 0; i < bufdata->textures_count; i++) {
    // Get heap required size for textures
    cgltf_buffer_view *bufferview = bufdata->textures[i].image->buffer_view;

    int32_t width, height, n;
    stbi_info_from_memory((void *)((uint64_t)bufferview->buffer->data + bufferview->offset),
      (int32_t)bufferview->size, &width, &height, &n);

    heaptotalsize += ALIGN_UP(TRUE_IMAGE_SIZE_IN_BYTES(width, height), 1024 * 64);
  }

  // Create normal heap and upload heap
  ret.heap = dx12_create_heap(dxstate, heaptotalsize, D3D12_HEAP_TYPE_DEFAULT);
  ret.uploadheap = dx12_create_heap(dxstate, heaptotalsize, D3D12_HEAP_TYPE_UPLOAD);

  uint64_t totalallocatedsize = 0; // This variable is to store the total size used up (so far) after each model is allocated.
                              // This is in order to ensure the correct offset and no overlapping

  for (uint32_t i = 0; i < bufdata->meshes_count; i++) {
    // Calculate the starting byte of each asset in the descriptorheap, and hence the total size required
    cgltf_buffer_view *indexbuffer = bufdata->meshes[i].primitives->indices->buffer_view;
    cgltf_buffer_view *vertexbuffer = bufdata->meshes[i].primitives->attributes[0].data->buffer_view;
    cgltf_buffer_view *texturebuffer = bufdata->meshes[i].primitives->attributes[2].data->buffer_view;
    cgltf_buffer_view *normalbuffer = bufdata->meshes[i].primitives->attributes[1].data->buffer_view;

    uint64_t vertexbufferoffset = totalallocatedsize;
    uint64_t texturebufferoffset = ALIGN_UP(vertexbufferoffset + vertexbuffer->size, 1024 * 64);
    uint64_t normalbufferoffset = ALIGN_UP(texturebufferoffset + texturebuffer->size, 1024 * 64);
    uint64_t indexbufferoffset = ALIGN_UP(normalbufferoffset + normalbuffer->size, 1024 * 64);
    uint64_t totalsize = ALIGN_UP(indexbufferoffset + indexbuffer->size, 1024 * 64);

    // Increment offset for the next model/texture
    totalallocatedsize = totalsize;

    // Create vertex, texture, and normal buffers, and index buffer for model
    ret.models[i + indexoffset].vertexbuffer = dx12_create_and_upload_vertex_buffer(dxstate, ret.heap, ret.uploadheap, (void *)((uint64_t)vertexbuffer->buffer->data + (uint64_t)vertexbuffer->offset), vertexbuffer->size, vertexbufferoffset, 3 * sizeof(float));

    ret.models[i + indexoffset].texturebuffer = dx12_create_and_upload_vertex_buffer(dxstate, ret.heap, ret.uploadheap, (void *)((uint64_t)texturebuffer->buffer->data + (uint64_t)texturebuffer->offset), texturebuffer->size, texturebufferoffset, 2 * sizeof(float));

    ret.models[i + indexoffset].normalbuffer = dx12_create_and_upload_vertex_buffer(dxstate, ret.heap, ret.uploadheap, (void *)((uint64_t)normalbuffer->buffer->data + (uint64_t)normalbuffer->offset), normalbuffer->size, normalbufferoffset, 3 * sizeof(float));

    ret.models[i + indexoffset].indexbuffer = dx12_create_and_upload_index_buffer(dxstate, ret.heap, ret.uploadheap, (void *)((uint64_t)indexbuffer->buffer->data + (uint64_t)indexbuffer->offset), indexbuffer->size, indexbufferoffset, DXGI_FORMAT_R16_UINT);

    ret.models[i + indexoffset].facecount = (uint32_t)indexbuffer->size / sizeof(uint16_t) / 3;

    // Get texture (if applicable)
    if (bufdata->meshes[i].primitives->material && bufdata->meshes[i].primitives->material->pbr_metallic_roughness.base_color_texture.texture) {
      ret.models[i + indexoffset].textureindex = bufdata->meshes[i].primitives->material->pbr_metallic_roughness.base_color_texture.texture - bufdata->textures + indexoffset;
    }
  }

  for (DWORD i = 0; i < bufdata->nodes_count; i++) {
    struct world_object *obj = &ret.objects[i + indexoffset];
    vec3_copy(&obj->transform.position, vec3_fromarray(bufdata->nodes[i].translation));
    vec3_copy(&obj->transform.rotation, vec3_fromarray(bufdata->nodes[i].rotation));
    vec3_copy(&obj->transform.scale, vec3_fromarray(bufdata->nodes[i].scale));
    obj->modelindex = bufdata->nodes[i].mesh - bufdata->meshes + indexoffset;
  }

  for (uint32_t i = 0; i < bufdata->textures_count; i++) {
    cgltf_buffer_view *bufferview = bufdata->textures[i].image->buffer_view;

    int32_t width, height, n;
    void *data = stbi_load_from_memory((void *)((uint64_t)bufferview->buffer->data + bufferview->offset), (int32_t)bufferview->size, &width, &height, &n, 4);

    // Use the totalallocatedsize variable in order to offset the texture in the heap
    struct dx12_texture texture = dx12_create_and_upload_texture(dxstate, width, height, ret.heap, ret.uploadheap, data, totalallocatedsize);

    // Free texture data
    stbi_image_free(data);

    // Create the shader buffer view in the descriptor heap
    D3D12_SHADER_RESOURCE_VIEW_DESC srvdesc = {0};
    srvdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvdesc.Texture2D.MipLevels = 1;

    // Create temporary variables in order not to affect function parameter values
    struct dx12_descriptor_handle handle = dx12_get_next_unused_handle(dxstate, heap);

    dxstate->device->lpVtbl->CreateShaderResourceView(dxstate->device, texture.texture, &srvdesc, handle.cpuhandle);

    totalallocatedsize += ALIGN_UP(TRUE_IMAGE_SIZE_IN_BYTES(width, height), 1024 * 64);

    ret.texture[i + indexoffset].handle = handle.gpuhandle;
  }

  // Free gltf data
  cgltf_free(bufdata);

  // Return
  return ret;
}
