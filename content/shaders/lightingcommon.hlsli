#ifndef LIGHTING_COMMON_HLSLI
#define LIGHTING_COMMON_HLSLI

#include "common.hlsli"
#include "random.hlsli"
#include "pbrutils.hlsli"
#include "sceneconstants.hlsli"

float3 ApplyDirectionalLighting(in ResolvedMaterial material, in float3 V, inout uint rngState, RaytracingAccelerationStructure RaytracingScene)
{    
    float3 result = float3(0.0f, 0.0f, 0.0f);
    for (uint i = 0; i < SceneConstants.NumDirectionalLightsInUse; ++i)
    {
        const DirectionalLightData dirLight = SceneConstants.DirectionalLights[i];
        if (all(dirLight.Emission < 1e-6f))
            continue;

        const float3 L = normalize(-dirLight.Direction);
            
        const float3 up = abs(L.y) < 0.999 ? float3(0, 1, 0) : float3(1, 0, 0);
        const float3 right = normalize(cross(up, L));
        const float3 forward = normalize(cross(L, right));
                
        RayDesc ray;
        ray.Direction = L;
        ray.TMin = 0.01f;
        ray.TMax = 1000000.0f;
    
        static const uint SamplesPerLight = 1;
        static const float SampleWeight = 1.0f / SamplesPerLight;
        
        float shadowFactor = 1.0f;
        for (uint i = 0; i < SamplesPerLight; ++i)
        {
            float2 xi;
            xi.x = RandomFloat01(rngState);
            xi.y = RandomFloat01(rngState);
            
            float2 diskSample = SampleUniformDisk(xi);

            // Transform disk sample into world space offset
            float3 offset = (diskSample.x * right + diskSample.y * forward) * dirLight.Radius;
            ray.Origin = material.WorldPosition + offset + material.WorldNormal * FLT_SMALL_VALUE;
            
            RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> rayQuery;       
            rayQuery.TraceRayInline(RaytracingScene, 0, 0xff, ray);
            rayQuery.Proceed();
        
            if (rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
            {
                shadowFactor = 0.0f;
                break;
            }
        }
        
        result += ComputeLuminance(material, V, L, dirLight.Emission) * shadowFactor;
    }         
    
    return result;
}

float3 ApplyLighting(in ResolvedMaterial material, in float3 V, inout uint rngState)
{
    RaytracingAccelerationStructure RaytracingScene = ResourceDescriptorHeap[SceneConstants.RaytracingSceneDescriptorIndex];
    float3 result = float3(0.0f, 0.0f, 0.0f);
    result += ApplyDirectionalLighting(material, V, rngState, RaytracingScene);
    return result;
}

#endif //LIGHTING_COMMON_HLSL