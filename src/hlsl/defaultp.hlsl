#include "default.hlsli"

Texture2D t0 : register(t0);
Texture2D t1 : register(t1);
SamplerState s0 : register(s0);

float4 main(VSOUT input) : SV_TARGET {
  VERTEXDATA data = vertexdata[offset];

  float4 colour;
  if (vertexdata[offset].colour.w > 0) {
    colour = vertexdata[offset].colour;
  } else {
    colour = t0.Sample(s0, float2(input.tex.x, 1 + input.tex.y));
  }

  return colour;
}
