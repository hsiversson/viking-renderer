#include "sceneconstants.hlsl"
#include "velocity.hlsl"

cbuffer ConstantBuffer : register(b0)
{
    uint DepthBufferDescriptorIndex;
    uint VelocityBufferDescriptorIndex;
};

[numthreads(8,8,1)]
void MainCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    Texture2D<float> DepthBuffer = ResourceDescriptorHeap[DepthBufferDescriptorIndex];
    RWTexture2D<float2> VelocityBuffer = ResourceDescriptorHeap[VelocityBufferDescriptorIndex];
    
    const uint2 pixel = dispatchThreadID.xy;
    float depth = DepthBuffer[pixel];
    float2 vel = VelocityBuffer[pixel];
    // If depth is 0 then we have a sky point. Otherwise if theres depth but velocity hasnt been written to means it was a static object that in the depth prepass didnt wrote velocity.
    bool computeVelocity = (depth == 0) || any(isnan(vel));
    if (computeVelocity)
    {
        // Compute velocity based only on camera movement (current/prev vieprojection)
        uint width;
        uint height;
        VelocityBuffer.GetDimensions(width, height);
        float2 uv = (pixel + 0.5f) / float2(width, height);
        uv.y = 1.0 - uv.y;
        float2 ndc = uv * 2.0f - 1.0f;
        float4 clipPos = float4(ndc, depth, 1.0);
        float4 worldPos = mul(SceneConstants.InvViewProjection, clipPos);
        worldPos /= worldPos.w;
        
        float4 prevClipPos = mul(SceneConstants.PrevViewProjection, worldPos);
        float2 velocity = CalcVelocity(clipPos,prevClipPos);
        VelocityBuffer[pixel] = velocity;
    }
}