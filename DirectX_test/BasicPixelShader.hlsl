#include "BasicShaderHeader.hlsli"

float4 BasicPS(Output input) : SV_TARGET
{
	//return float4(output.uv, 1.0f, 1.0f);
    return float4(tex.Sample(smp, input.uv));
}