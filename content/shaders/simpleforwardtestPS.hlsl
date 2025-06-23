cbuffer ConstantBuffer : register(b0)
{
	float4x4 ViewProjection;
	float4x4 World;
	float4 BaseColor;
};

struct PSInput
{
	float3 InPosition : POSITION;
	float4 Position : SV_POSITION;
};

float4 MainPS(PSInput input) : SV_TARGET
{
	//return BaseColor;
	return float4(input.InPosition * 0.5 + 0.5, 1.0f);
}