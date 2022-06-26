struct PSInput {
  float4 position : SV_POSITION;
  float4 colour : COLOUR;
};


PSInput VertexEntry(float3 position : POSITION, float4 colour : COLOUR) {
  PSInput ret;
  ret.position = float4(position, 1);
  ret.colour = colour;
  return ret;
}

float4 PixelEntry(PSInput input) : SV_TARGET {
  return input.colour;
}
