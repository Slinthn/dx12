WINRESOURCE WINLoadResource(U32 name, U32 type) {
  HRSRC src = FindResource(0, MAKEINTRESOURCE(name), MAKEINTRESOURCE(type));
  HGLOBAL global = LoadResource(0, src);
  void *data = LockResource(global);
  U32 size = SizeofResource(0, src);

  WINRESOURCE res = {0};
  res.src = src;
  res.global = global;
  res.data = data;
  res.size = size;
  return res;
}

WORLD LoadGLTF(WINSTATE *winstate, void *gltfdata, U64 sizeinbytes, DX12DESCRIPTORHEAP *heap) {
  WORLD ret = {0};

  U64 indexoffset = 1; // Make the first entry in each array (objects, textures, etc) a null entry

  DX12STATE *dxstate = &winstate->dxstate;

  cgltf_options options = {0};
  cgltf_data* bufdata;
  cgltf_parse(&options, gltfdata, sizeinbytes, &bufdata);
  cgltf_load_buffers(&options, bufdata, 0);

  U64 heaptotalsize = 0; // This variable is to store the size of the WHOLE allocated heap (to be used later when allocating memory for the heap)
#if 1

  for (U32 i = 0; i < bufdata->meshes_count; i++) {
    // Get heap required size for models
    cgltf_buffer_view *indexbuffer = bufdata->meshes[i].primitives->indices->buffer_view;
    cgltf_buffer_view *vertexbuffer = bufdata->meshes[i].primitives->attributes[0].data->buffer_view;
    cgltf_buffer_view *texturebuffer = bufdata->meshes[i].primitives->attributes[2].data->buffer_view;
    cgltf_buffer_view *normalbuffer = bufdata->meshes[i].primitives->attributes[1].data->buffer_view;

    U64 vertexbufferoffset = 0;
    U64 texturebufferoffset = AlignUp(vertexbufferoffset + vertexbuffer->size, 1024 * 64);
    U64 normalbufferoffset = AlignUp(texturebufferoffset + texturebuffer->size, 1024 * 64);
    U64 indexbufferoffset = AlignUp(normalbufferoffset + normalbuffer->size, 1024 * 64);
    heaptotalsize += AlignUp(indexbufferoffset + indexbuffer->size, 1024 * 64);
  }
#endif // 0


  for (U32 i = 0; i < bufdata->textures_count; i++) {
    // Get heap required size for textures
    cgltf_buffer_view *bufferview = bufdata->textures[i].image->buffer_view;

    S32 width, height, n;
    stbi_info_from_memory((void *)((U64)bufferview->buffer->data + bufferview->offset), (S32)bufferview->size, &width, &height, &n);

    heaptotalsize += AlignUp(TrueImageSizeInBytes(width, height), 1024 * 64);
  }

  // Create normal heap and upload heap
  ret.heap = DXCreateHeap(dxstate, heaptotalsize, D3D12_HEAP_TYPE_DEFAULT);
  ret.uploadheap = DXCreateHeap(dxstate, heaptotalsize, D3D12_HEAP_TYPE_UPLOAD);

  U64 totalallocatedsize = 0; // This variable is to store the total size used up (so far) after each model is allocated.
                                 // This is in order to ensure the correct offset and no overlapping

  for (U32 i = 0; i < bufdata->meshes_count; i++) {
    // Calculate the starting byte of each asset in the descriptorheap, and hence the total size required
    cgltf_buffer_view *indexbuffer = bufdata->meshes[i].primitives->indices->buffer_view;
    cgltf_buffer_view *vertexbuffer = bufdata->meshes[i].primitives->attributes[0].data->buffer_view;
    cgltf_buffer_view *texturebuffer = bufdata->meshes[i].primitives->attributes[2].data->buffer_view;
    cgltf_buffer_view *normalbuffer = bufdata->meshes[i].primitives->attributes[1].data->buffer_view;

    U64 vertexbufferoffset = totalallocatedsize;
    U64 texturebufferoffset = AlignUp(vertexbufferoffset + vertexbuffer->size, 1024 * 64);
    U64 normalbufferoffset = AlignUp(texturebufferoffset + texturebuffer->size, 1024 * 64);
    U64 indexbufferoffset = AlignUp(normalbufferoffset + normalbuffer->size, 1024 * 64);
    U64 totalsize = AlignUp(indexbufferoffset + indexbuffer->size, 1024 * 64);

    // Increment offset for the next model/texture
    totalallocatedsize = totalsize;

    // Create vertex, texture, and normal buffers, and index buffer for model
    ret.models[i + indexoffset].vertexbuffer = DXCreateAndUploadVertexBuffer(dxstate, ret.heap, ret.uploadheap, (void *)((U64)vertexbuffer->buffer->data + (U64)vertexbuffer->offset), vertexbuffer->size, vertexbufferoffset, 3 * sizeof(float));

    ret.models[i + indexoffset].texturebuffer = DXCreateAndUploadVertexBuffer(dxstate, ret.heap, ret.uploadheap, (void *)((U64)texturebuffer->buffer->data + (U64)texturebuffer->offset), texturebuffer->size, texturebufferoffset, 2 * sizeof(float));

    ret.models[i + indexoffset].normalbuffer = DXCreateAndUploadVertexBuffer(dxstate, ret.heap, ret.uploadheap, (void *)((U64)normalbuffer->buffer->data + (U64)normalbuffer->offset), normalbuffer->size, normalbufferoffset, 3 * sizeof(float));

    ret.models[i + indexoffset].indexbuffer = DXCreateAndUploadIndexBuffer(dxstate, ret.heap, ret.uploadheap, (void *)((U64)indexbuffer->buffer->data + (U64)indexbuffer->offset), indexbuffer->size, indexbufferoffset, DXGI_FORMAT_R16_UINT);

    ret.models[i + indexoffset].facecount = (U32)indexbuffer->size / sizeof(U16) / 3;

    // Get texture (if applicable)
    if (bufdata->meshes[i].primitives->material && bufdata->meshes[i].primitives->material->pbr_metallic_roughness.base_color_texture.texture) {
      ret.models[i + indexoffset].textureindex = bufdata->meshes[i].primitives->material->pbr_metallic_roughness.base_color_texture.texture - bufdata->textures + indexoffset;
    }
  }

  for (DWORD i = 0; i < bufdata->nodes_count; i++) {
    WORLDOBJECT *obj = &ret.objects[i + indexoffset];
    VECCopy3f(&obj->transform.position, bufdata->nodes[i].translation);
    VECCopy3f(&obj->transform.rotation, bufdata->nodes[i].rotation);
    VECCopy3f(&obj->transform.scale, bufdata->nodes[i].scale);
    obj->modelindex = bufdata->nodes[i].mesh - bufdata->meshes + indexoffset;
  }

  for (U32 i = 0; i < bufdata->textures_count; i++) {
    cgltf_buffer_view *bufferview = bufdata->textures[i].image->buffer_view;

    S32 width, height, n;
    void *data = stbi_load_from_memory((void *)((U64)bufferview->buffer->data + bufferview->offset), (S32)bufferview->size, &width, &height, &n, 4);

    // Use the totalallocatedsize variable in order to offset the texture in the heap
    DX12TEXTURE texture = DXCreateAndUploadTexture(dxstate, width, height, ret.heap, ret.uploadheap, data, totalallocatedsize);

    // Free texture data
    stbi_image_free(data);

    // Create the shader buffer view in the descriptor heap
    D3D12_SHADER_RESOURCE_VIEW_DESC srvdesc = {0};
    srvdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvdesc.Texture2D.MipLevels = 1;

    // Create temporary variables in order not to affect function parameter values
    DX12DESCRIPTORHANDLE handle = DXGetNextUnusedHandle(dxstate, heap);

    dxstate->device->lpVtbl->CreateShaderResourceView(dxstate->device, texture.texture, &srvdesc, handle.cpuhandle);

    totalallocatedsize += AlignUp(TrueImageSizeInBytes(width, height), 1024 * 64);

    ret.texture[i + indexoffset].handle = handle.gpuhandle;
  }

  // Free gltf data
  cgltf_free(bufdata);

  // Return
  return ret;
}