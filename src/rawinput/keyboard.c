void rawinput_parse_keyboard_data(user_controls *control, RAWINPUT *data) {
  u8 down = !(data->data.keyboard.Flags & RI_KEY_BREAK);
  u16 key = data->data.keyboard.VKey;

  switch (key) {
  case 'W': {
    control->move[1] = (float)-down;
  } break;

  case 'S': {
    control->move[1] = down;
  } break;

  case 'D': {
    control->move[0] = down;
  } break;

  case 'A': {
    control->move[0] = (float)-down;
  } break;
  
  case VK_SHIFT: {
    if (down) {
      control->actions |= ACTION_DESCEND;
    } else {
      control->actions ^= ACTION_DESCEND;
    }
  } break;

  case ' ': {
    if (down) {
      control->actions |= ACTION_ASCEND;
    } else {
      control->actions ^= ACTION_ASCEND;
    }
  } break;
  }
}