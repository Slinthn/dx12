#define EXIT_ERROR_CODE_INVALID_SM 0x1
#define EXIT_ERROR_CODE_INVALID_SW 0x2

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

#define TrueImageSizeInBytes(width, height) (AlignUp((width), 256) * (height) * 4)

#pragma pack(push, 1)
typedef struct t_PERVERTEXDATA {
  MATRIX transform;
  VECTOR4F colour;
  VECTOR4F unused0[3];
} PERVERTEXDATA;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct t_CB0 {
  PERVERTEXDATA data[100];
} CB0;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct t_CB2 {
  MATRIX perspective;
  MATRIX camera;
  MATRIX sun;
} CB2;
#pragma pack(pop)

typedef struct {
  ID3D12Heap *heap, *uploadheap;
  ID3D12Resource *constantbuffer0, *constantbuffer0upload;
  DX12DESCRIPTORHANDLE constantbuffer0handle;
  ID3D12Resource *constantbuffer2, *constantbuffer2upload;
  DX12DESCRIPTORHANDLE constantbuffer2handle;
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
  UBYTE unused0[4];
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
  DX12SHADER defaultshader;
  DX12SHADER shadershader;
  WORLD world1;

  DX12SAMPLER sampler;
  HEAP0 heap0;
  DX12DESCRIPTORHEAP heap;
  ID3D12DescriptorHeap *shaderdescriptorheap;
  ID3D12Resource *shaderdepthresource;

  DX12DESCRIPTORHANDLE shadertexturehandle;

  PLAYER player;
  TRANSFORM sun;
  UBYTE unused1[4];
} WINSTATE;