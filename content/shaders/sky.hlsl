// Valhalla awaits!!

cbuffer PerBatchConstantBuffer : register(b0)
{
    uint BackbufferDescriptorIndex;
    uint DepthbufferDescriptorIndex;
};

[numthreads(8,8,1)]
void MainCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    static const int ColorCount = 6;
    float4 sunsetColors[ColorCount] =
    {
        float4(0.05, 0.10, 0.30, 1.0), // Deep blue (dusk sky)
    float4(0.35, 0.10, 0.40, 1.0), // Violet
    float4(0.70, 0.20, 0.50, 1.0), // Magenta/pink
    float4(0.95, 0.45, 0.30, 1.0), // Orange
    float4(1.00, 0.70, 0.30, 1.0), // Golden orange
    float4(1.00, 0.85, 0.60, 1.0) // Pale yellow/gold (sun glow)
    };
    
    RWTexture2D<float4> BackbufferTexture = ResourceDescriptorHeap[ BackbufferDescriptorIndex];
    Texture2D<float> DepthTexture = ResourceDescriptorHeap[DepthbufferDescriptorIndex];
    
    uint width, height;
    BackbufferTexture.GetDimensions(width, height);
    
    if (dispatchThreadID.x >= width || dispatchThreadID.y >= height)
        return;
    
    float depth = DepthTexture.Load(int3(dispatchThreadID.xy, 0)).r;
    if (depth == 0.0f)
    {
        float t = dispatchThreadID.y / (float) (height - 1);
        t = saturate(t);
        float scaled = t * (ColorCount - 1);
        int i = (int) floor(scaled);
        float localT = frac(scaled);
        i = clamp(i, 0, ColorCount - 2);
        float4 SunsetColor = lerp(sunsetColors[i], sunsetColors[i + 1], localT);
        
        BackbufferTexture[dispatchThreadID.xy] = SunsetColor;
    }

}