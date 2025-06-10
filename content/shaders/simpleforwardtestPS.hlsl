cbuffer ConstantBuffer : register(b0)
{
    float4x4 WorldViewProjection;
    float4 BaseColor;
};

struct PSInput
{
    float4 Position : SV_POSITION;
};

float4 MainPS(PSInput input) : SV_TARGET
{
    return BaseColor;
}