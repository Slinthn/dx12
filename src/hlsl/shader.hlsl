#include "shader.hlsli"

[RootSignature(ROOTSIGNATURE)]
VSOUT VertexEntry(float3 position : POSITION, float3 normal : NORMAL, float2 tex : TEXTURE) {
  VSOUT ret;
  VERTEXDATA data = vertexdata[offset];
  ret.position = mul(float4(position, 1.0f), data.transform);
  ret.position = mul(ret.position, suncamera);
  ret.position = mul(ret.position, sunperspective);
  return ret;
}

void PixelEntry(VSOUT input) {
}