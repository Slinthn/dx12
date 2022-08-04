// APP

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

// MATH

typedef float MATRIX[16];
typedef float VECTOR2F[2];
typedef float VECTOR3F[3];

#define PIf 3.1415926535897932384626f
#define DToR(x) ((x * PIf) / 180.0f)

typedef struct {
  VECTOR3F position;
  VECTOR3F rotation;
} TRANSFORM;

typedef struct {
  TRANSFORM camera;
} GAMESTATE;

// RAWINPUT

#define ACTION_INTERACT 0x1

#pragma pack(push, 1)
typedef struct {
  unsigned char reportid;
  unsigned char lx;
  unsigned char ly;
  unsigned char rx;
  unsigned char ry;
  unsigned short buttons;
  unsigned char counter;
  unsigned char l2;
  unsigned char r2;
} DS4;
#pragma pack(pop)

typedef struct {
  VECTOR2F move;
  VECTOR2F look;
  unsigned char actions;
  char unused[3];
} CONTROL;

// DIRECT3D 12

#pragma pack(push, 1)
typedef struct {
  MATRIX perspective;
  MATRIX transform;
  MATRIX camera;
  MATRIX nothings[1];
} CB0;
#pragma pack(pop)

typedef struct {
  void *cbptrs[10]; // TODO max 10 here
  ID3D12DescriptorHeap *cbvheap;
  ID3D12RootSignature *rootsignature;
  ID3D12PipelineState *pipeline;
} DXSHADER;

typedef struct {
  ID3D12Debug *debug;
  IDXGIFactory4 *factory;
  IDXGIAdapter *adapter;
  ID3D12Device *device;
  ID3D12InfoQueue *infoqueue;
  ID3D12CommandQueue *queue;
  IDXGISwapChain3 *swapchain;
  ID3D12DescriptorHeap *rtvheap;
  ID3D12Resource *rendertargets[2];
  ID3D12CommandAllocator *allocator;
  ID3D12GraphicsCommandList *list;
  ID3D12Fence *fence;
  HANDLE fenceevent;
  unsigned long long fencevalue;
  ID3D12DescriptorHeap *dsvheap;
} DX12STATE;

typedef struct {
  ID3D12Resource *buffer;
  D3D12_VERTEX_BUFFER_VIEW view;
} DX12VERTEXBUFFER;

typedef struct {
  ID3D12Resource *buffer;
  D3D12_INDEX_BUFFER_VIEW view;
} DX12INDEXBUFFER;

#pragma pack(push, 1)
typedef struct {
  float position[3];
  float normal[3];
  float texture[2];
} VERTEX;
#pragma pack(pop)

// FILE

#pragma pack(push, 1)
typedef struct {
  char signature[2];
  unsigned int vertexcount;
  unsigned int facecount;
} SMHEADER;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
  SMHEADER header;
  VERTEX *vertices;
  unsigned int *indices;
} SM;
#pragma pack(pop)

// WINDOWS

#define EXIT_ERROR_CODE_INVALID_SM 0x1

#define SizeofArray(x) (sizeof(x) / sizeof((x)[0]))

typedef struct {
  HRSRC src;
  HGLOBAL global;
  void *data;
  unsigned int size;
} WRESOURCE;

typedef struct {
  CONTROL controls; 
} WSTATE;