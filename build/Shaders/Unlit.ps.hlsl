#include "Types.h"

float4 PS(PSInput input) : SV_Target
{
  return txDiffuse.Sample(samLinear, input.tex);
}