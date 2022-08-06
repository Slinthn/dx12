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

SMODEL WINLoadSModel(WINRESOURCE res) {
  SMHEADER *header = (SMHEADER *)res.data;
  if (CompareStringA(LOCALE_CUSTOM_DEFAULT, 0, header->signature, 2, "SM", 2) != CSTR_EQUAL) { // TODO does this work?
    ExitProcess(EXIT_ERROR_CODE_INVALID_SM);
  }

  SMODEL ret = {0};
  ret.header = *header;
  ret.vertices = (VERTEX *)((UQWORD)res.data + (UQWORD)sizeof(SMHEADER));
  ret.indices = (UDWORD *)((UQWORD)ret.vertices + (UQWORD)(ret.header.vertexcount * sizeof(VERTEX)));
  return ret;
}

SWORLD WINLoadSWorld(WINRESOURCE res) {
  SWHEADER *header = (SWHEADER *)res.data;
  if (CompareStringA(LOCALE_CUSTOM_DEFAULT, 0, header->signature, 2, "SW", 2) != CSTR_EQUAL) { // TODO does this work?
    ExitProcess(EXIT_ERROR_CODE_INVALID_SW);
  }

  SWORLD ret = {0};
  ret.header = *header;
  ret.models = (SWMODEL *)(((UQWORD)header) + sizeof(SWHEADER));
  ret.objects = (SWOBJECT *)(((UQWORD)ret.models) + header->modelcount * sizeof(SWMODEL));

  return ret;
}

WINMODEL WINLoadModel(DX12STATE *dxstate, SMODEL smodel) {
  WINMODEL ret = {0};

  ret.model = smodel;
  ret.vertexbuffer = DXCreateVertexBuffer(dxstate, smodel.vertices, sizeof(VERTEX), smodel.header.vertexcount * sizeof(VERTEX));
  ret.indexbuffer = DXCreateIndexBuffer(dxstate, smodel.indices, smodel.header.facecount * 3 * sizeof(unsigned int));

  return ret;
}