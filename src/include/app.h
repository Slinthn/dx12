#define EXIT_ERROR_CODE_INVALID_SM 0x1
#define EXIT_ERROR_CODE_INVALID_SW 0x2

#define WINDOW_WIDTH 852
#define WINDOW_HEIGHT 480

#define SHADOW_WIDTH 400
#define SHADOW_HEIGHT 400

#define TRUE_IMAGE_SIZE_IN_BYTES(width, height) (ALIGN_UP((width), 256) * (height) * 4)

#pragma pack(push, 1)
typedef struct t_per_vertex_data {
  mat4 transform;
  vec4 colour;
  vec4 unused0[3];
} per_vertex_data;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct t_constant_buffer0 {
  per_vertex_data data[100];
} constant_buffer0;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct t_constant_buffer2 {
  mat4 cameraperspective;
  mat4 camera;
  mat4 suncamera;
  mat4 sunperspective;
} constant_buffer2;
#pragma pack(pop)

typedef struct t_heap0 {
  ID3D12Heap *heap, *uploadheap;
  ID3D12Resource *constantbuffer0, *constantbuffer0upload;
  dx12_descriptor_handle constantbuffer0handle;
  ID3D12Resource *constantbuffer2, *constantbuffer2upload;
  dx12_descriptor_handle constantbuffer2handle;
} heap0;

typedef struct t_win64_resource {
  HRSRC src;
  HGLOBAL global;
  void *data;
  u32 size;
  u8 unused0[4];
} win64_resource;

typedef struct t_win64_texture {
  D3D12_GPU_DESCRIPTOR_HANDLE handle;
} win64_texture;

typedef struct t_dx12_model {
  u32 facecount;
  u8 unused0[4];
  u64 textureindex;
  dx12_vertexbuffer vertexbuffer;
  dx12_vertexbuffer texturebuffer;
  dx12_vertexbuffer normalbuffer;
  dx12_indexbuffer indexbuffer;
  vec4 colour;
} dx12_model;

typedef struct t_world_object {
  u64 modelindex;
  transformation transform;
} world_object;

typedef struct t_world {
  ID3D12Heap *heap, *uploadheap;
  dx12_model models[100]; // TODO 100 max.
  win64_texture texture[100];
  world_object objects[100];
} world;

typedef struct t_player {
  transformation transform;
  vec3 velocity;
} player;

typedef struct t_win64_state {
  dx12_state dxstate;
  user_controls controls;
  u8 unused0[4];
  dx12_shader defaultshader;
  dx12_shader shadershader;
  world world1;

  dx12_sampler sampler;
  heap0 heap0;
  dx12_descriptor_heap descriptorheap;

  dx12_shadow shadow;

  player player;
  transformation sun;
  u8 unused1[4];
} win64_state;
