#define EXIT_ERROR_CODE_INVALID_SM 0x1
#define EXIT_ERROR_CODE_INVALID_SW 0x2

#define WINDOW_WIDTH 852
#define WINDOW_HEIGHT 480

#define SHADOW_WIDTH 400
#define SHADOW_HEIGHT 400

#define TRUE_IMAGE_SIZE_IN_BYTES(width, height) (ALIGN_UP((width), 256) * (height) * 4)

#pragma pack(push, 1)
struct per_vertex_data {
  mat4 transform;
  struct vector4 colour;
  struct vector4 unused0[3];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct constant_buffer0 {
  struct per_vertex_data data[100];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct constant_buffer2 {
  mat4 cameraperspective;
  mat4 camera;
  mat4 suncamera;
  mat4 sunperspective;
};
#pragma pack(pop)

struct win64_resource {
  HRSRC src;
  HGLOBAL global;
  void *data;
  uint32_t size;
  uint8_t unused0[4];
};

struct win64_texture {
  D3D12_GPU_DESCRIPTOR_HANDLE handle;
};

struct dx12_model {
  uint32_t facecount;
  uint8_t unused0[4];
  uint64_t textureindex;
  struct dx12_vertexbuffer vertexbuffer;
  struct dx12_vertexbuffer texturebuffer;
  struct dx12_vertexbuffer normalbuffer;
  struct dx12_indexbuffer indexbuffer;
  struct vector4 colour;
};

struct world_object {
  uint64_t modelindex;
  struct transformation transform;
};

struct world {
  ID3D12Heap *heap, *uploadheap;
  struct dx12_model models[100]; // TODO 100 max.
  struct win64_texture texture[100];
  struct world_object objects[100];
};

struct player {
  struct transformation transform;
  struct vector3 velocity;
};

struct win64_state {
  struct dx12_state dxstate;
  struct user_controls controls;
  uint8_t unused0[4];
  struct dx12_shader defaultshader;
  struct dx12_shader shadershader;
  struct world world1;

  struct dx12_sampler sampler;
  ID3D12Heap *heap, *uploadheap;
  ID3D12Resource *constantbuffer0, *constantbuffer0upload;
  struct dx12_descriptor_handle constantbuffer0handle;
  ID3D12Resource *constantbuffer2, *constantbuffer2upload;
  struct dx12_descriptor_handle constantbuffer2handle;

  struct dx12_descriptor_heap descriptorheap;

  struct dx12_shadow shadow;

  struct player player;
  struct transformation sun;
  uint8_t unused1[4];
};
