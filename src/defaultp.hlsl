#include "default.hlsli"

Texture2D t0 : register(t0);
SamplerState s0 : register(s0);

cbuffer CB1 : register(b1) {
  int offset;
};

[RootSignature(ROOTSIGNATURE)]
float4 main(PSINPUT input) : SV_TARGET {
  if (offset == 2) {
    float2 pos = float2(input.tex.x, 1 + input.tex.y);
    return t0.Sample(s0, pos);
  } else {
    return float4(0, 0, 0, 1);
  }
}
