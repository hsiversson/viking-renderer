// Valhalla awaits!!

cbuffer PerBatchConstantBuffer : register(b0)
{
    uint BackbufferDescriptorIndex;
    uint DepthbufferDescriptorIndex;
};

[numthreads(8,8,1)]
void MainCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    RWTexture2D<float4> BackbufferTexture = ResourceDescriptorHeap[ BackbufferDescriptorIndex];
    Texture2D<float> DepthTexture = ResourceDescriptorHeap[DepthbufferDescriptorIndex];
    
    uint width, height;
    BackbufferTexture.GetDimensions(width, height);
    
    if (dispatchThreadID.x >= width || dispatchThreadID.y >= height)
        return;
    
    float depth = DepthTexture.Load(int3(dispatchThreadID.xy, 0)).r;
    if (depth == 0.0f)
        BackbufferTexture[dispatchThreadID.xy] = float4(0, 0, 1, 1);

}