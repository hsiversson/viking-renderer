cbuffer ConstantBuffer : register(b0)
{
	float4x4 ViewProjection;
	float4x4 World;
    float3 BaseColor;
    uint TextureDescriptor;
};

struct VSInput
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float2 UV : UV;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float2 UV : UV;
};

VSOutput MainVS(VSInput input)
{
	VSOutput output;
	float4 worldpos = mul(World, float4(input.Position, 1.0));
	output.Position = mul(ViewProjection, worldpos);
    output.Normal = input.Normal;
    output.UV = input.UV;
	return output;
}