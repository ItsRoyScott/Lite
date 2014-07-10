#include "Types.h"

float4 PS(PSInput input) : SV_Target
{
  float4 finalColor = (float4)0;
  
  for (int i = 0; i < 2; ++i)
  {
    float4 NDotL = dot((float3)cbLightDirections[i], input.normal);
    finalColor += saturate(NDotL * cbLightColors[i]);
  }
  finalColor.a = 1;
  
  return finalColor * txDiffuse.Sample(samLinear, input.tex);
}