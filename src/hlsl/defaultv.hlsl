#include "default.hlsli"

[RootSignature(ROOTSIGNATURE)]
VSOUT main(float3 position : POSITION, float3 normal : NORMAL, float2 tex : TEXTURE) {
  VSOUT ret;
  VERTEXDATA data = vertexdata[offset];
  ret.worldposition = mul(float4(position, 1.0f), data.transform);
  ret.position = mul(ret.worldposition, data.camera);
  ret.position = mul(ret.position, data.perspective);
  ret.normal = normal;
  ret.tex = tex;
  return ret;
}
