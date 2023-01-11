void rawinput_parse_keyboard_data(
  struct user_controls *control,
  RAWINPUT *data)
{
  uint8_t down = !(data->data.keyboard.Flags & RI_KEY_BREAK);
  uint16_t key = data->data.keyboard.VKey;

  switch (key) {
  case 'W': {
    control->move.y = (float)-down;
  } break;

  case 'S': {
    control->move.y = down;
  } break;

  case 'D': {
    control->move.x = down;
  } break;

  case 'A': {
    control->move.x = (float)-down;
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