#define ACTION_INTERACT 0x1

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