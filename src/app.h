#define EXIT_ERROR_CODE_INVALID_SM 0x1
#define EXIT_ERROR_CODE_INVALID_SW 0x2

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

#define TrueImageSizeInBytes(width, height) (AlignUp((width), 256) * (height) * 4)

#pragma pack(push, 1)
typedef struct {
  MATRIX perspective;
  MATRIX transform;
  MATRIX camera;
  VECTOR4F colour;
  VECTOR4F unused0[3];
} CB0;
#pragma pack(pop)

typedef struct {
  ID3D12Heap *heap, *uploadheap;
  ID3D12Resource *constantbuffer0, *constantbuffer0upload;
} HEAP0;

typedef struct {
  HRSRC src;
  HGLOBAL global;
  void *data;
  UDWORD size;
  UBYTE unused0[4];
} WINRESOURCE;

typedef struct {
  D3D12_GPU_DESCRIPTOR_HANDLE handle;
} WINTEXTURE;

typedef struct {
  UDWORD facecount;
  UQWORD textureindex;
  DX12VERTEXBUFFER vertexbuffer;
  DX12VERTEXBUFFER texturebuffer;
  DX12VERTEXBUFFER normalbuffer;
  DX12INDEXBUFFER indexbuffer;
  VECTOR4F colour;
} WINMODEL;

typedef struct {
  UQWORD modelindex;
  TRANSFORM transform;
} WORLDOBJECT;

typedef struct {
  ID3D12Heap *heap, *uploadheap;
  WINMODEL models[100]; // TODO 100 max.
  WINTEXTURE texture[100];
  WORLDOBJECT objects[100];
} WORLD;

typedef struct {
  TRANSFORM transform;
  VECTOR3F velocity;
} PLAYER;

typedef struct {
  DX12STATE dxstate;
  CONTROL controls;
  UBYTE unused0[4];
  DX12SHADER shader;
  WORLD world1;

  DX12SAMPLER sampler;
  HEAP0 heap0;
  ID3D12DescriptorHeap *descriptorheap;

  PLAYER player;
  UBYTE unused1[4];
} WINSTATE;