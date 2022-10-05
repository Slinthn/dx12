void rawinput_parse_mouse_data(user_controls *control, RAWINPUT *data) {
  control->look[0] = (float)data->data.mouse.lLastX;
  control->look[1] = (float)data->data.mouse.lLastY;
}