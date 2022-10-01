#include "default.hlsli"

[RootSignature(ROOTSIGNATURE)]
VSOUT VertexEntry(float3 position : POSITION, float3 normal : NORMAL, float2 tex : TEXTURE) {
  VSOUT ret;
  VERTEXDATA data = vertexdata[offset];
  ret.worldposition = mul(float4(position, 1.0f), data.transform);
  ret.position = mul(ret.worldposition, camera);
  ret.position = mul(ret.position, cameraperspective);
  ret.normal = normal;
  ret.tex = tex;
  return ret;
}


Texture2D t0 : register(t0);
Texture2D t1 : register(t1);
SamplerState s0 : register(s0);

float4 PixelEntry(VSOUT input) : SV_TARGET {
  VERTEXDATA data = vertexdata[offset];

  float4 fragcoord = mul(mul(input.worldposition, suncamera), sunperspective);
  float3 fragconstrained = fragcoord.xyz / fragcoord.w;

  fragconstrained = fragconstrained * 0.5 + 0.5;

  float lit = 1;
  if (fragconstrained.x > 0 && fragconstrained.x < 1 && fragconstrained.y > 0 && fragconstrained.y < 1) {
    float4 zvalue = t1.Sample(s0, float2(fragconstrained.x, 1 - fragconstrained.y));
    if (abs(fragcoord.z - zvalue.x) < 0.0002f) { // TODO this is 100% certified SCUFFED
      lit = 1;
    } else {
      lit = 0.1;
    }
  }

  float4 colour;
  if (vertexdata[offset].colour.w > 0) {
    colour = vertexdata[offset].colour;
  } else {
    colour = t0.Sample(s0, float2(input.tex.x, 1 + input.tex.y));
  }

  return colour * lit;
}
