#include "common.hlsli"

struct VSOUT {
  float4 position : SV_POSITION;
  float4 worldposition : WORLDPOSITION;
  float3 normal : NORMAL;
  float2 tex : TEXTURE;
};

#define ROOTSIGNATURE \
  "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)," \
  "DescriptorTable(CBV(b0, flags = DATA_VOLATILE))," \
  "RootConstants(num32BitConstants = 1, b1)," \
  "DescriptorTable(SRV(t0))," \
  "DescriptorTable(SRV(t1))," \
  "DescriptorTable(CBV(b2, flags = DATA_VOLATILE))," \
  "StaticSampler(s0)"