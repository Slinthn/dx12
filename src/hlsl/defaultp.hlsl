#include "default.hlsli"

Texture2D t0 : register(t0);
SamplerState s0 : register(s0);

float4 main(VSOUT input) : SV_TARGET {
  VERTEXDATA data = vertexdata[offset];
  float4 position = input.worldposition;
  float4 lightpos = float4(100, 100, 100, 0);

  float4 direction = normalize(lightpos - position);
  float strength = dot(direction.xyz, input.normal);

  float4 colour;
  if (vertexdata[offset].colour.w > 0) {
    colour = vertexdata[offset].colour;
  } else {
    colour = t0.Sample(s0, float2(input.tex.x, 1 + input.tex.y));
  }

  colour *= strength;
  return colour;
}
