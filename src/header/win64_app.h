#define EXIT_ERROR_CODE_INVALID_SM 0x1
#define EXIT_ERROR_CODE_INVALID_SW 0x2

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

typedef struct {
  HRSRC src;
  HGLOBAL global;
  void *data;
  UDWORD size;
  UBYTE unused0[4];
} WINRESOURCE;

typedef struct {
  SMODEL model;
  UWORD modelid;
  UBYTE unused0[6];
  DX12VERTEXBUFFER vertexbuffer;
  DX12INDEXBUFFER indexbuffer;
} WINMODEL;

typedef struct {
  DX12STATE dxstate;
  CONTROL controls;
  TRANSFORM camera;
  UBYTE unused0[4];
  DXSHADER shader;
  WINMODEL models[100]; // TODO 100 max.
  SWORLD world1;
} WINSTATE;