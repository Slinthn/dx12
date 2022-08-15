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