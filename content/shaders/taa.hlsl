#include "sceneconstants.hlsl"

cbuffer PerBatchConstantBuffer : register(b0)
{
    uint ResolveTextureDescriptorIndex;
    uint SceneTextureDescriptorIndex;
    uint HistoryTextureDescriptorIndex;
    uint VelocityTextureDescriptorIndex;
};

SamplerState g_SamplerPointClamp : register(s0);
SamplerState g_SamplerBilinearClamp : register(s1);

[numthreads(8,8,1)]
void ResolveCS(uint3 dispatchThreadID:SV_DispatchThreadID)
{
    RWTexture2D<float4> ResolveTexture = ResourceDescriptorHeap[ResolveTextureDescriptorIndex];
    Texture2D<float4> SceneTexture = ResourceDescriptorHeap[SceneTextureDescriptorIndex];
    Texture2D<float4> HistoryTexture = ResourceDescriptorHeap[HistoryTextureDescriptorIndex];
    Texture2D<float2> VelocityTexture = ResourceDescriptorHeap[VelocityTextureDescriptorIndex];
    
    uint width, height;
    HistoryTexture.GetDimensions(width, height);
    const float2 uv = (dispatchThreadID.xy + 0.5) / float2(width,height);
    
    const float2 velocity = VelocityTexture.Sample(g_SamplerPointClamp, uv);
    float2 prevPixelPos = uv - velocity;
    
    const float3 sceneColor = SceneTexture[dispatchThreadID.xy].rgb;
    float3 historyColor = HistoryTexture.Sample(g_SamplerBilinearClamp, prevPixelPos).rgb;
    
    const float modulationFactor = 0.9f;
    
    float3 finalColor = lerp(sceneColor, historyColor, modulationFactor);
    
    ResolveTexture[dispatchThreadID.xy] = float4(finalColor, 1.0f);
}