void GameInit(WINSTATE *winstate) {
  DX12STATE *dxstate = &winstate->dxstate;

  winstate->world1 = WINLoadSWorld(WINLoadResource(WORLD1, WORLD));

  for (UDWORD i = 0; i < winstate->world1.header.modelcount; i++) {
    SWMODEL *model = &winstate->world1.models[i];
    winstate->models[model->modelid] = WINLoadModel(dxstate, WINLoadSModel(WINLoadResource(model->modelid, MODEL)));
  }

  D3D12_INPUT_ELEMENT_DESC ieds[] =
  {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  };

  WINRESOURCE vertex = WINLoadResource(DEFAULT_VERTEX, VERTEXSHADER);
  WINRESOURCE pixel = WINLoadResource(DEFAULT_PIXEL, PIXELSHADER);

  UDWORD cbsizes[] = {sizeof(CB0) * 100};
  winstate->shader = DXCreateShader(dxstate, SizeofArray(cbsizes), cbsizes, vertex.data, vertex.size, pixel.data, pixel.size, ieds, SizeofArray(ieds));
}

void GameUpdate(WINSTATE *winstate) {
  DX12STATE *dxstate = &winstate->dxstate;

  DXEnableShader(dxstate, winstate->shader);
  DXPrepareFrame(dxstate);
  
  VECTOR2F look;
  VECCopy2f(&look, winstate->controls.look);
  VECMul2f(&look, (VECTOR2F){0.1f, 0.1f});

  winstate->camera.rotation[1] += look[0];
  winstate->camera.rotation[0] += look[1];

  VECTOR2F move;
  VECCopy2f(&move, winstate->controls.move);
  if (VECMagnitude2f(move) > 1.0f) {
    VECNormalise2f(&move);
  }

  VECMul2f(&move, (VECTOR2F){0.1f, 0.1f});

  float rotcos = cosf(winstate->camera.rotation[1]);
  float rotsin = sinf(winstate->camera.rotation[1]);

  VECTOR2F movedir;
  movedir[0] = move[0] * rotcos - move[1] * rotsin;
  movedir[1] = -move[0] * rotsin - move[1] * rotcos;

  winstate->camera.position[0] += movedir[0];
  winstate->camera.position[2] += movedir[1];

  dxstate->list->lpVtbl->IASetPrimitiveTopology(dxstate->list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  for (UDWORD i = 0; i < winstate->world1.header.objectcount; i++) {
    SWOBJECT *obj = &winstate->world1.objects[i];
    
    CB0 data = {0};
    MPerspective(&data.perspective, DegreesToRadians(100.0f), 0.1f, 100.0f);
    MTransform(&data.transform, obj->position[0], obj->position[1], obj->position[2], obj->rotation[0], obj->rotation[1], obj->rotation[2]);
    MTransform(&data.camera, winstate->camera.position[0], winstate->camera.position[1], winstate->camera.position[2], winstate->camera.rotation[0], winstate->camera.rotation[1], winstate->camera.rotation[2]);

    CopyMemory((void *)((UQWORD)winstate->shader.cbptrs[0] + sizeof(CB0) * i), &data, sizeof(CB0));

    dxstate->list->lpVtbl->SetGraphicsRoot32BitConstant(dxstate->list, 1, i, 0);

    dxstate->list->lpVtbl->IASetVertexBuffers(dxstate->list, 0, 1, &winstate->models[obj->modelid].vertexbuffer.view);
    dxstate->list->lpVtbl->IASetIndexBuffer(dxstate->list, &winstate->models[obj->modelid].indexbuffer.view);
    dxstate->list->lpVtbl->DrawIndexedInstanced(dxstate->list, winstate->models[obj->modelid].model.header.facecount * 3, 1, 0, 0, 0);
  }
  
  DXFlushFrame(dxstate);
}