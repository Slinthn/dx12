void rawinput_parse_dualshock4_data(
  struct user_controls *control,
  struct rawinput_dualshock4 *data)
{
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

  control->move.x = (data->lx - 127.5f) / 127.5f;
  control->move.y = (data->ly - 127.5f) / 127.5f;
  control->look.x = (data->rx - 127.5f) / 127.5f;
  control->look.y = (data->ry - 127.5f) / 127.5f;
}