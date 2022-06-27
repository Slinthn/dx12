struct PSInput {
  float4 position : SV_POSITION;
  float4 colour : COLOUR;
};

cbuffer CB0 : register(b0) {
  float4x4 perspective;
};


PSInput VertexEntry(float3 position : POSITION, float4 colour : COLOUR) {
  PSInput ret;
  ret.position = mul(float4(position, 1), perspective);
  ret.colour = colour;
  return ret;
}

float4 PixelEntry(PSInput input) : SV_TARGET {
  return input.colour;
}
