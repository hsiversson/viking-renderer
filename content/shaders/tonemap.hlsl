#include "colorcommon.hlsli"

struct ConstantsStruct
{
    uint TargetDescriptorIndex;
    uint TonemapType;
    uint EncodingType;
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
    
    float3 linearRgb = target[pixel].rgb; // in working color space (ACEScg)
    
    // transform to target color space
    
    
    // perform tonemap
    float3 tonemapped = Tonemap(linearRgb);
    
    float3 encoded;
    if (Constants.EncodingType == DISPLAY_ENCODING_TYPE_SRGB)
    {
        encoded = EncodeSRGB(tonemapped);
    }
    else if (Constants.EncodingType == DISPLAY_ENCODING_TYPE_ST2048)
    {
        encoded = EncodeSt2048(tonemapped);
    }
    else if (Constants.EncodingType == DISPLAY_ENCODING_TYPE_HLG)
    {
        encoded = EncodeHLG(tonemapped);
    }
    
    target[pixel] = float4(encoded, 0.0f);
}