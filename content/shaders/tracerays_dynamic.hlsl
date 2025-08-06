#include "common.hlsl"
#include "raytracingcommon.hlsl"
#include "sceneconstants.hlsl"

struct ConstantsStruct
{
    uint TargetTextureDescriptorIndex;
    uint NormalsTargetTextureDescriptorIndex;
    uint2 pad;
};
ConstantBuffer<ConstantsStruct> Constants : register(b0);

[shader("raygeneration")]
void TraceRays()
{
    const uint2 pixel = DispatchRaysIndex().xy;
    
    RWTexture2D<float4> target = ResourceDescriptorHeap[Constants.TargetTextureDescriptorIndex];
    uint width, height;
    target.GetDimensions(width, height);
    
    const float2 texelSize = 1.0f / float2(width, height);
    const float2 uv = (pixel + 0.5f) * texelSize;
    const float2 ndc = float2(uv.x * 2.0f - 1.0f, (1.0f - uv.y) * 2.0f - 1.0f);
    
    //Unproject pixel to get ray origin and direction
    float4 clipPos = float4(ndc, 0.0f, 1.0f);
    float4 worldPos = mul(SceneConstants.InvViewProjection, clipPos);
    worldPos /= worldPos.w;
    float3 ro = SceneConstants.CameraPosition;
    float3 rd = normalize(worldPos.xyz - SceneConstants.CameraPosition);
    
    RayDesc ray;
    ray.Origin = ro;
    ray.Direction = rd;
    ray.TMin = 0.001f;
    ray.TMax = 1000000.0f;
    
    RaytracingAccelerationStructure RaytracingScene = ResourceDescriptorHeap[SceneConstants.RaytracingSceneDescriptorIndex];
    
    uint flags = RAY_FLAG_NONE;
    flags |= RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES;
    
    RaytracingPayload payload = (RaytracingPayload)0;
    payload.rngState = GenerateRandomSeed(pixel.x, pixel.y, SceneConstants.FrameIndex);
    
    TraceRay(RaytracingScene, flags, 0xff, 0, 0, 0, ray, payload);

    target[pixel] = float4(payload.irradiance, 1.0f);
    
    // write normals for denoising
    RWTexture2D<float4> normalsTarget = ResourceDescriptorHeap[Constants.NormalsTargetTextureDescriptorIndex];
    normalsTarget[pixel] = float4(payload.worldNormal, 0.0f);
}

[shader("miss")]
void Miss(inout RaytracingPayload payload)
{
    // Sample sky
    static const int NumSkyColors = 6;
    static const float3 SkyColors[NumSkyColors] =
    {
        float3(0.05, 0.10, 0.30), // Deep blue (dusk sky)
        float3(0.35, 0.10, 0.40), // Violet
        float3(0.70, 0.20, 0.50), // Magenta/pink
        float3(0.95, 0.45, 0.30), // Orange
        float3(1.00, 0.70, 0.30), // Golden orange
        float3(1.00, 0.85, 0.60) // Pale yellow/gold (sun glow)
    };
        
    const float3 rd = WorldRayDirection();
        
    float t = saturate(1.0 - abs(dot(rd, float3(0, 1, 0))));
    float scaled = t * (NumSkyColors - 1);
    
    int i = clamp((int)floor(scaled), 0, NumSkyColors - 2);
    
    float localT = frac(scaled);
    float3 skyColor = lerp(SkyColors[i], SkyColors[i + 1], localT);
        
    // Calculate simple sun disk
    for (uint lightIdx = 0; lightIdx < SceneConstants.NumDirectionalLightsInUse; ++lightIdx)
    {
        const DirectionalLightData dirLight = SceneConstants.DirectionalLights[lightIdx];

        float sunDot = dot(rd, -normalize(dirLight.Direction)); // cos(angle)
        float cosInner = cos(dirLight.Radius); // Hard edge
        float cosOuter = cos(dirLight.Radius * 2.0f); // Feathered falloff

        float sunFactor = saturate((sunDot - cosOuter) / (cosInner - cosOuter));

        // Optionally apply power falloff for a softer edge
        sunFactor = pow(sunFactor, 4.0); // tweak this for sharpness
            
        float3 sunDiskColor = dirLight.Emission * sunFactor;
        skyColor += sunDiskColor;
    }
        
    payload.irradiance = skyColor;
}