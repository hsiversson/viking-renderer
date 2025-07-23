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

static const uint GROUP_SIZE = 8;
static const uint TILE_BORDER = 1;
static const uint TILE_SIZE = GROUP_SIZE + 2 * TILE_BORDER;
static const uint TILE_CACHE_SIZE = TILE_SIZE * TILE_SIZE;

//Stores the scene colors for this group of threads serialized by rows
groupshared float3 GroupColorCache[TILE_CACHE_SIZE];

[numthreads(GROUP_SIZE, GROUP_SIZE, 1)]
void ResolveCS(uint3 dispatchThreadID:SV_DispatchThreadID, uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID, uint groupIdx : SV_GroupIndex)
{
    RWTexture2D<float4> ResolveTexture = ResourceDescriptorHeap[ResolveTextureDescriptorIndex];
    Texture2D<float4> SceneTexture = ResourceDescriptorHeap[SceneTextureDescriptorIndex];
    Texture2D<float4> HistoryTexture = ResourceDescriptorHeap[HistoryTextureDescriptorIndex];
    Texture2D<float2> VelocityTexture = ResourceDescriptorHeap[VelocityTextureDescriptorIndex];
    
    //Fill in the cache, this will accelerate later the access to neighborhoods
    uint2 startTexel = groupID.xy * GROUP_SIZE - TILE_BORDER;
    for (uint t = groupIdx; t < TILE_CACHE_SIZE; t += GROUP_SIZE * GROUP_SIZE)
    {
        const uint2 samplePixel = startTexel + uint2(t % TILE_SIZE, t / TILE_SIZE);
        float3 color = SceneTexture[samplePixel].rgb;
        GroupColorCache[t] = color;
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    uint width, height;
    HistoryTexture.GetDimensions(width, height);
    const float2 uv = (dispatchThreadID.xy + 0.5) / float2(width,height);
    
    const float2 velocity = VelocityTexture.Sample(g_SamplerPointClamp, uv);
    float2 prevPixelPos = uv - velocity;
    
    const float3 sceneColor = SceneTexture[dispatchThreadID.xy].rgb;
    float3 historyColor = HistoryTexture.Sample(g_SamplerBilinearClamp, prevPixelPos).rgb;
    
    //Neighborhood clamping.
    float3 neighborhoodMin = 100000;
    float3 neighborhoodMax = -100000;
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            const int2 offset = int2(x, y);
            const uint2 tileIndex = groupThreadID.xy + TILE_BORDER + offset;
            const uint cacheIdx = tileIndex.y * TILE_SIZE + tileIndex.x;

            float3 color = GroupColorCache[cacheIdx];
            neighborhoodMin = min(neighborhoodMin, color);
            neighborhoodMax = max(neighborhoodMax, color);

        }
    }
    float3 historyColorClamped = clamp(historyColor, neighborhoodMin, neighborhoodMax);
    
    const float modulationFactor = 0.9f;
    
    float3 finalColor = lerp(sceneColor, historyColorClamped, modulationFactor);
    
    ResolveTexture[dispatchThreadID.xy] = float4(finalColor, 1.0f);
}