#define ACTION_JUMP (0x1)
#define ACTION_DESCEND (0x2)
#define ACTION_ASCEND (0x4)

#pragma pack(push, 1)
typedef struct {
  U8 reportid;
  U8 lx;
  U8 ly;
  U8 rx;
  U8 ry;
  U16 buttons;
  U8 counter;
  U8 l2;
  U8 r2;
} DS4;
#pragma pack(pop)

typedef struct {
  VECTOR2F move;
  VECTOR2F look;
  U8 actions;
  S8 unused[3];
} CONTROL;
