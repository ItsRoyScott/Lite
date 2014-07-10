Texture2D txDiffuse : register( t0 );
SamplerState samLinear : register( s0 );

cbuffer SceneConstants : register(b0)
{
  matrix cbViewProjection;
  float4 cbOutputColor;
  float4 cbLightDirections[2];
  float4 cbLightColors[2];
}

cbuffer ObjectConstants : register(b1)
{
  matrix cbWorld;
}

struct VSInput
{
  float4 position : POSITION;
  float3 normal : NORMAL;
  float2 tex : TEXCOORD0;
};

struct PSInput
{
  float4 position : SV_POSITION;
  float3 normal : TEXCOORD0; 
  float2 tex : TEXCOORD1;
};
