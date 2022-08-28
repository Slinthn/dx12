#include "shader.hlsli"

[RootSignature(ROOTSIGNATURE)]
VSOUT main(float3 position : POSITION, float3 normal : NORMAL, float2 tex : TEXTURE) {
  VSOUT ret;
  VERTEXDATA data = vertexdata[offset];
  ret.position = mul(float4(position, 1.0f), data.transform);
  ret.position = mul(ret.position, camera);
  ret.position = mul(ret.position, perspective);
  return ret;
}
