#define ACTION_JUMP 0x1
#define ACTION_DESCEND 0x2
#define ACTION_ASCEND 0x4

#pragma pack(push, 1)
typedef struct {
  UBYTE reportid;
  UBYTE lx;
  UBYTE ly;
  UBYTE rx;
  UBYTE ry;
  UWORD buttons;
  UBYTE counter;
  UBYTE l2;
  UBYTE r2;
} DS4;
#pragma pack(pop)

typedef struct {
  VECTOR2F move;
  VECTOR2F look;
  UBYTE actions;
  SBYTE unused[3];
} CONTROL;