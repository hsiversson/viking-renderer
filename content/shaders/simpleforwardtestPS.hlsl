cbuffer ConstantBuffer : register(b0)
{
	float4x4 ViewProjection;
	float4x4 World;
	float3 BaseColor;
    uint TextureDescriptor;
};

SamplerState g_SamplerPointClamp : register(s0);
SamplerState g_SamplerBilinearClamp : register(s1);

struct PSInput
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float2 UV : UV;
};

float4 MainPS(PSInput input) : SV_TARGET
{
    Texture2D tex = ResourceDescriptorHeap[TextureDescriptor];
    float4 texColor = tex.Sample(g_SamplerBilinearClamp, input.UV);
    return float4(texColor.rgb, 1.0f);
}