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
    float4 totalz = 0;
    for (int x = -1; x < 2; x++) {
      for (int y = -1; y < 2; y++) {
        float xcoord = fragconstrained.x + 0.001f * x;
        float ycoord = 1 - (fragconstrained.y + 0.001f * y);
        totalz += t1.Sample(s0, float2(xcoord, ycoord));
      }
    }

    float4 averagez = totalz / 9.0f;

    if (abs(fragcoord.z - averagez.x) < 0.00015f) { // TODO this is 100% certified SCUFFED
      lit = 1;
    } else {
      lit = 0.2;
    }
  }

  float4 colour;
  if (vertexdata[offset].colour.w > 0) {
    colour = vertexdata[offset].colour;
  } else {
    colour = t0.Sample(s0, float2(input.tex.x, 1 + input.tex.y));
  }

  colour *= lit;

  float grayscale = 0.2989 * colour.x + 0.587 * colour.y + 0.114 * colour.z;

  //return float4(grayscale, grayscale, grayscale, 1);
  return colour;
}
