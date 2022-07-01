void RIInit(HWND window) {
  RAWINPUTDEVICE device[1] = {0};
  device[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
  device[0].usUsage = HID_USAGE_GENERIC_GAMEPAD;
  device[0].hwndTarget = window;
  RegisterRawInputDevices(device, SizeofArray(device), sizeof(RAWINPUTDEVICE));
}

#pragma pack(push, 1)
typedef struct {
  unsigned char reportid;
  unsigned char lx;
  unsigned char ly;
  unsigned char rx;
  unsigned char ry;
  unsigned short buttons;
  unsigned char counter;
  unsigned char l2;
  unsigned char r2;
} DS4;
#pragma pack(pop)

void RIParse(HRAWINPUT rawinput) {
  unsigned int size;
  GetRawInputData(rawinput, RID_INPUT, 0, &size, sizeof(RAWINPUTHEADER));

  RAWINPUT *data = VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  GetRawInputData(rawinput, RID_INPUT, data, &size, sizeof(RAWINPUTHEADER));

  DS4 *ds4data = (DS4 *)data->data.hid.bRawData;

  VirtualFree(data, 0, MEM_RELEASE);
}
