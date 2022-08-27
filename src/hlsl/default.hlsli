struct VSOUT {
  float4 position : SV_POSITION;
  float4 worldposition : WORLDPOSITION;
  float3 normal : NORMAL;
  float2 tex : TEXTURE;
};

struct VERTEXDATA {
  float4x4 perspective;
  float4x4 transform;
  float4x4 camera;
  float4 colour;
  float4 unused0[3];
};

cbuffer CB0 : register(b0) {
  VERTEXDATA vertexdata[100];
};

cbuffer CB1 : register(b1) {
  int offset;
};

#define ROOTSIGNATURE \
  "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)," \
  "DescriptorTable(CBV(b0, flags = DATA_VOLATILE))," \
  "RootConstants(num32BitConstants = 1, b1)," \
  "DescriptorTable(SRV(t0))," \
  "StaticSampler(s0)"