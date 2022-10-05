void rawinput_parse_dualshock4_data(user_controls *control, rawinput_dualshock4 *data) {
 control->actions = 0;
  if (data->buttons & (1 << 5)) {
    control->actions |= ACTION_JUMP;
  }

  if (data->buttons & (1 << 10)) {
    control->actions |= ACTION_DESCEND;
  }

  if (data->buttons & (1 << 11)) {
    control->actions |= ACTION_ASCEND;
  }

  control->move[0] = (data->lx - 127.5f) / 127.5f;
  control->move[1] = (data->ly - 127.5f) / 127.5f;
  control->look[0] = (data->rx - 127.5f) / 127.5f;
  control->look[1] = (data->ry - 127.5f) / 127.5f;
}