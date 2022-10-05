#define ACTION_JUMP (0x1)
#define ACTION_DESCEND (0x2)
#define ACTION_ASCEND (0x4)

#pragma pack(push, 1)
typedef struct t_rawinput_dualshock4 {
  u8 reportid;
  u8 lx;
  u8 ly;
  u8 rx;
  u8 ry;
  u16 buttons;
  u8 counter;
  u8 l2;
  u8 r2;
} rawinput_dualshock4;
#pragma pack(pop)

typedef struct t_user_controls {
  vec2 move;
  vec2 look;
  u8 actions;
  u8 unused[3];
} user_controls;
