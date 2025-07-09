#include "../../../content/shaders/sceneconstants.hlsl"
#include "../../../content/shaders/instancing.hlsl"

cbuffer PerBatchConstantBuffer : register(b0)
{
    uint BatchInstanceDataOffsetStart;
};

struct InstanceData
{
    float4x4 WorldTransform;
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

VSOutput MainVS(VSInput input, uint instanceID : SV_InstanceID)
{
	
	VSOutput output;
	
    InstanceData data = GetInstanceData<InstanceData>(BatchInstanceDataOffsetStart, instanceID);
	
	float4 worldpos = mul(data.WorldTransform, float4(input.Position, 1.0));
	output.Position = mul(ViewProjection, worldpos);
    output.Normal = input.Normal;
    output.UV = input.UV;
	return output;
}