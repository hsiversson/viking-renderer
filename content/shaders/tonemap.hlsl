#include "colorcommon.hlsli"

struct ConstantsStruct
{
    uint TargetDescriptorIndex;
    uint TonemapType;
    uint SourceColorSpace;
    uint TargetColorSpace;
};
ConstantBuffer<ConstantsStruct> Constants : register(b0);

static const uint TONEMAP_TYPE_REINHARD = 0;
static const uint TONEMAP_TYPE_ACES_APPROX = 1;
static const uint TONEMAP_TYPE_AGX_APPROX = 2;
static const uint TONEMAP_TYPE_HABLE = 3;
static const uint TONEMAP_TYPE_GRAN_TURISMO = 4;

float3 Tonemap(float3 linearRgb)
{
    // TODO: Make sure to handle HDR vs SDR compression correctly.
    
    float3 tonemapped;
    
    if (Constants.TonemapType == TONEMAP_TYPE_REINHARD)
    {
        tonemapped = linearRgb / (1.0f + linearRgb);
    }
    else if (Constants.TonemapType == TONEMAP_TYPE_ACES_APPROX)
    {
        float a = 2.51f;
        float b = 0.03f;
        float c = 2.43f;
        float d = 0.59f;
        float e = 0.14f;
        tonemapped = saturate((linearRgb * (a * linearRgb + b)) / (linearRgb * (c * linearRgb + d) + e));
    }
    else if (Constants.TonemapType == TONEMAP_TYPE_AGX_APPROX)
    {
    }
    else if (Constants.TonemapType == TONEMAP_TYPE_HABLE)
    {
    }
    else if (Constants.TonemapType == TONEMAP_TYPE_GRAN_TURISMO)
    {
    }
    else
    {
        tonemapped = linearRgb;
    }
    
    return tonemapped;
}

[numthreads(8,8,1)]
void Main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint2 pixel = dispatchThreadId.xy;
    
    RWTexture2D<float4> target = ResourceDescriptorHeap[Constants.TargetDescriptorIndex];
    
    float exposure = 0.25f; // TODO: use calculated exposure instead.
    float3 linearRgb = target[pixel].rgb * exposure;
    
    const ColorSpace sourceColorSpace = COLOR_SPACES[Constants.SourceColorSpace];
    const ColorSpace targetColorSpace = COLOR_SPACES[Constants.TargetColorSpace];
    
    // transform to target color space
    linearRgb = TransformColor(linearRgb, sourceColorSpace, targetColorSpace);
    
    // perform tonemap
    float3 tonemapped = Tonemap(linearRgb);
    
    float3 encoded = EncodeColor(saturate(tonemapped), targetColorSpace);
    
    target[pixel] = float4(saturate(encoded), 1.0f);
}