cbuffer ConstantBuffer : register(b0)
{
	float4x4 ViewProjection;
	float4x4 World;
	float4 BaseColor;
};

struct VSInput
{
	float3 Position : POSITION;
};

struct VSOutput
{
	float4 Position : SV_Position;
};

VSOutput MainVS(VSInput input)
{
	VSOutput output;
	float4 worldpos = mul(World, float4(input.Position, 1.0));
	output.Position = mul(ViewProjection, worldpos);
	return output;
}