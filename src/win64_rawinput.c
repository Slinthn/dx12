void RIInit(HWND window) {
  RAWINPUTDEVICE device[2] = {0};
  device[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
  device[0].usUsage = HID_USAGE_GENERIC_GAMEPAD;
  device[0].hwndTarget = window;
  device[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
  device[1].usUsage = HID_USAGE_GENERIC_KEYBOARD;
  device[1].hwndTarget = window;
  RegisterRawInputDevices(device, SizeofArray(device), sizeof(RAWINPUTDEVICE));
}

void RIParse(CONTROL *control, HRAWINPUT rawinput) {
  unsigned int size;
  GetRawInputData(rawinput, RID_INPUT, 0, &size, sizeof(RAWINPUTHEADER));

  RAWINPUT *data = VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  GetRawInputData(rawinput, RID_INPUT, data, &size, sizeof(RAWINPUTHEADER));
  
  switch (data->header.dwType) {
  case RIM_TYPEKEYBOARD: {
    unsigned char down = !(data->data.keyboard.Flags & RI_KEY_BREAK);
    unsigned short key = data->data.keyboard.VKey;

    switch (key) {
    case 'W': {
      control->move[1] = down;
    } break;

    case 'S': {
      control->move[1] = (float)-down;
    } break;

    case 'D': {
      control->move[0] = down;
    } break;

    case 'A': {
      control->move[0] = (float)-down;
    } break;
    }
  } break;
  
  case RIM_TYPEHID: {
    DS4 *ds4data = (DS4 *)data->data.hid.bRawData;
    if (ds4data->buttons & (1 << 5)) {
      control->actions |= ACTION_INTERACT;
    }

    control->move[0] = (ds4data->lx - 127.5f) / 127.5f;
    control->move[1] = (ds4data->ly - 127.5f) / 127.5f;
    control->look[0] = (ds4data->rx - 127.5f) / 127.5f;
    control->look[1] = (ds4data->ry - 127.5f) / 127.5f;
  } break;
  }

  VirtualFree(data, 0, MEM_RELEASE);
}
