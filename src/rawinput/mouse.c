void rawinput_parse_mouse_data(
  struct user_controls *control,
  RAWINPUT *data)
{
  control->look.x = (float)data->data.mouse.lLastX;
  control->look.y = (float)data->data.mouse.lLastY;
}