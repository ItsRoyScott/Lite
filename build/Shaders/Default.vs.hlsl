#include "Types.h"

PSInput VS(VSInput input)
{
  PSInput output = (PSInput) 0;
  output.position = mul(input.position, cbWorld);
  output.position = mul(output.position, cbViewProjection);
  output.normal = mul(input.normal, cbWorld);
  output.tex = input.tex;
  
  return output;
}
