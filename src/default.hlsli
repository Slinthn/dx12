struct PSINPUT {
  float4 position : SV_POSITION;
  float3 normal : NORMAL;
  float2 tex : TEXTURE;
};

#define ROOTSIGNATURE "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), DescriptorTable(CBV(b0, flags = DATA_VOLATILE)), RootConstants(num32BitConstants = 1, b1), DescriptorTable(SRV(t0)), StaticSampler(s0)"