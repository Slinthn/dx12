struct VERTEXDATA {
  float4x4 transform;
  float4 colour;
  float4 unused0[3];
};

cbuffer CB0 : register(b0) {
  VERTEXDATA vertexdata[100];
};

cbuffer CB1 : register(b1) {
  int offset;
};

cbuffer CB2 : register(b2) {
  float4x4 cameraperspective;
  float4x4 camera;
  float4x4 suncamera;
  float4x4 sunperspective;
};