WINRESOURCE WINLoadResource(UDWORD name, UDWORD type) {
  HRSRC src = FindResource(0, MAKEINTRESOURCE(name), MAKEINTRESOURCE(type));
  HGLOBAL global = LoadResource(0, src);
  void *data = LockResource(global);
  UDWORD size = SizeofResource(0, src);

  WINRESOURCE res = {0};
  res.src = src;
  res.global = global;
  res.data = data;
  res.size = size;
  return res;
}

WORLD LoadGLTF(WINSTATE *winstate, void *gltfdata, UQWORD sizeinbytes, D3D12_CPU_DESCRIPTOR_HANDLE cpudescriptorhandle, D3D12_GPU_DESCRIPTOR_HANDLE gpudescriptorhandle) {
  WORLD ret = {0};

  UQWORD indexoffset = 1; // Make the first entry in each array (objects, textures, etc) a null entry

  DX12STATE *dxstate = &winstate->dxstate;

  cgltf_options options = {0};
  cgltf_data* bufdata;
  cgltf_parse(&options, gltfdata, sizeinbytes, &bufdata);
  cgltf_load_buffers(&options, bufdata, 0);

  UQWORD heaptotalsize = 0; // This variable is to store the size of the WHOLE allocated heap (to be used later when allocating memory for the heap)
  for (UDWORD i = 0; i < bufdata->meshes_count; i++) {
    // Get heap required size for models
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

  for (UDWORD i = 0; i < bufdata->textures_count; i++) {
    // Get heap required size for textures
    cgltf_buffer_view *bufferview = bufdata->textures[i].image->buffer_view;

    SDWORD width, height, n;
    stbi_info_from_memory((void *)((UQWORD)bufferview->buffer->data + bufferview->offset), (SDWORD)bufferview->size, &width, &height, &n);

    heaptotalsize += TrueImageSizeInBytes(width, height);
  }

  // Create normal heap and upload heap
  ret.heap = DXCreateHeap(dxstate, heaptotalsize, D3D12_HEAP_TYPE_DEFAULT);
  ret.uploadheap = DXCreateHeap(dxstate, heaptotalsize, D3D12_HEAP_TYPE_UPLOAD);

  UQWORD totalallocatedsize = 0; // This variable is to store the total size used up (so far) after each model is allocated.
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

    // Increment offset for the next model/texture
    totalallocatedsize = totalsize;

    // Create vertex, texture, and normal buffers, and index buffer for model
    ret.models[i + indexoffset].vertexbuffer = DXCreateAndUploadVertexBuffer(dxstate, ret.heap, ret.uploadheap, (void*)((UQWORD)vertexbuffer->buffer->data + (UQWORD)vertexbuffer->offset), vertexbuffer->size, vertexbufferoffset, 3 * sizeof(float));

    ret.models[i + indexoffset].texturebuffer = DXCreateAndUploadVertexBuffer(dxstate, ret.heap, ret.uploadheap, (void*)((UQWORD)texturebuffer->buffer->data + (UQWORD)texturebuffer->offset), texturebuffer->size, texturebufferoffset, 2 * sizeof(float));

    ret.models[i + indexoffset].normalbuffer = DXCreateAndUploadVertexBuffer(dxstate, ret.heap, ret.uploadheap, (void*)((UQWORD)normalbuffer->buffer->data + (UQWORD)normalbuffer->offset), normalbuffer->size, normalbufferoffset, 3 * sizeof(float));

    ret.models[i + indexoffset].indexbuffer = DXCreateAndUploadIndexBuffer(dxstate, ret.heap, ret.uploadheap, (void*)((UQWORD)indexbuffer->buffer->data + (UQWORD)indexbuffer->offset), indexbuffer->size, indexbufferoffset, DXGI_FORMAT_R16_UINT);

    ret.models[i + indexoffset].facecount = (UDWORD)indexbuffer->size / sizeof(UWORD) / 3;

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

  for (UDWORD i = 0; i < bufdata->textures_count; i++) {
    cgltf_buffer_view *bufferview = bufdata->textures[i].image->buffer_view;

    SDWORD width, height, n;
    void *data = stbi_load_from_memory((void *)((UQWORD)bufferview->buffer->data + bufferview->offset), (SDWORD)bufferview->size, &width, &height, &n, 4);

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
    D3D12_CPU_DESCRIPTOR_HANDLE cpudescriptorhandletmp = cpudescriptorhandle;
    D3D12_GPU_DESCRIPTOR_HANDLE gpudescriptorhandletmp = gpudescriptorhandle;

    DWORD increment = dxstate->device->lpVtbl->GetDescriptorHandleIncrementSize(dxstate->device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * i; // Offset
    cpudescriptorhandletmp.ptr += increment;
    gpudescriptorhandletmp.ptr += increment;

    dxstate->device->lpVtbl->CreateShaderResourceView(dxstate->device, texture.texture, &srvdesc, cpudescriptorhandletmp);

    totalallocatedsize += AlignUp(TrueImageSizeInBytes(width, height), 1024 * 64);

    ret.texture[i + indexoffset].handle = gpudescriptorhandletmp;
  }

  // Free gltf data
  cgltf_free(bufdata);

  // Return
  return ret;
}