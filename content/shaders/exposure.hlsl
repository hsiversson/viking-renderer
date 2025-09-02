#include "common.hlsli"
#include "colorcommon.hlsli"

static const uint NumBins = 256;

struct ConstantsStruct
{
    uint2 RenderSize;
    uint ExposureTargetDescriptorIndex;
    uint HistogramDescriptorIndex;
    
    float DeltaTime;
    float MinLog; // -12 - Lowest f-Stop
    float MaxLog; // 20 - Highest f-Stop
    uint SceneColorDescriptorIndex;
    
    uint SceneColorSpace;
    uint3 unused;
};
ConstantBuffer<ConstantsStruct> Constants : register(b0);

//#if defined(CLEAR_HISTOGRAM)
[numthreads(NumBins, 1, 1)]
void ClearHistogram(uint binIndex : SV_GroupIndex)
{
    RWStructuredBuffer<uint> Histogram = ResourceDescriptorHeap[Constants.HistogramDescriptorIndex];
    Histogram[binIndex] = 0;
}
//#endif //CLEAR_HISTOGRAM

//#if defined(BUILD_HISTOGRAM)
[numthreads(8, 8, 1)]
void BuildHistogram(uint3 dtid : SV_DispatchThreadID)
{
    const uint2 pixel = dtid.xy;
    if (pixel.x >= Constants.RenderSize.x || pixel.y >= Constants.RenderSize.y)
        return;
    
    Texture2D<float4> Scene = ResourceDescriptorHeap[Constants.SceneColorDescriptorIndex];
    float3 sceneRGB = Scene.Load(int3(pixel, 0));
    
    float luma = GetLuminance(sceneRGB, COLOR_SPACES[Constants.SceneColorSpace]);
    luma = max(luma, 1e-6);

    float logLuma = log2(luma);
    float normalized = saturate((logLuma - Constants.MinLog) / (Constants.MaxLog - Constants.MinLog));
    uint bin = clamp((uint) (normalized * NumBins), 0, NumBins - 1);

    RWStructuredBuffer<uint> Histogram = ResourceDescriptorHeap[Constants.HistogramDescriptorIndex];
    InterlockedAdd(Histogram[bin], 1);
}
//#endif //BUILD_HISTOGRAM

//#if defined(COMPUTE_EXPOSURE)
groupshared uint HistogramLDS[NumBins];

[numthreads(NumBins, 1, 1)]
void ComputeExposure(uint groupIndex : SV_GroupIndex)
{
    const uint binIndex = groupIndex;
    
    RWStructuredBuffer<uint> Histogram = ResourceDescriptorHeap[Constants.HistogramDescriptorIndex];
    HistogramLDS[binIndex] = Histogram[binIndex];
    GroupMemoryBarrierWithGroupSync();

    for (uint offset = 1; offset < NumBins; offset <<= 1)
    {
        GroupMemoryBarrierWithGroupSync();
        uint val = 0;
        if (binIndex >= offset)
            val = HistogramLDS[binIndex - offset];
        GroupMemoryBarrierWithGroupSync();
        HistogramLDS[binIndex] += val;
    }
    GroupMemoryBarrierWithGroupSync();
    
    if (binIndex == 0)
    {
        // scan LDS sequentially to find percentile bin
        static const float percentile = 0.8;
        uint targetCount = (uint)(percentile * HistogramLDS[NumBins - 1]);
        uint percentileBin = 0;
        for (uint i = 0; i < NumBins; ++i)
        {
            if (HistogramLDS[i] >= targetCount)
            {
                percentileBin = i;
                break;
            }
        }

        // map bin to linear luminance
        float normalizedBin = (percentileBin + 0.5f) / NumBins;
        float logLuminance = lerp(Constants.MinLog, Constants.MaxLog, normalizedBin);
        
        static const float middleGray = 0.18f;
        float targetExposure = middleGray / exp2(logLuminance);

        // temporal smoothing
        RWTexture2D<float> exposureTarget = ResourceDescriptorHeap[Constants.ExposureTargetDescriptorIndex];
        float prevExposure = exposureTarget[int2(0, 0)];
        float adaptedExposure = prevExposure + (targetExposure - prevExposure) * (1.0f - exp(-Constants.DeltaTime * 1.5));
        exposureTarget[int2(0, 0)] = adaptedExposure;
    }
}
//#endif //COMPUTE_EXPOSURE