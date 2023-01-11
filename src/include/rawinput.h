#define ACTION_JUMP (0x1)
#define ACTION_DESCEND (0x2)
#define ACTION_ASCEND (0x4)

#pragma pack(push, 1)
struct rawinput_dualshock4 {
  uint8_t reportid;
  uint8_t lx;
  uint8_t ly;
  uint8_t rx;
  uint8_t ry;
  uint16_t buttons;
  uint8_t counter;
  uint8_t l2;
  uint8_t r2;
};
#pragma pack(pop)

struct user_controls {
  struct vector2 move;
  struct vector2 look;
  uint8_t actions;
  uint8_t unused[3];
};
