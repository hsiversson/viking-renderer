#ifndef LIGHTING_COMMON_HLSL
#define LIGHTING_COMMON_HLSL

#include "common.hlsl"
#include "pbrutils.hlsl"
#include "sceneconstants.hlsl"

float2 SampleDisk(float2 xi)
{
    float r = sqrt(xi.x);
    float theta = 2.0f * PI * xi.y;
    return float2(r * cos(theta), r * sin(theta));
}

void CreateOrthonormalBasis(float3 N, out float3 T, out float3 B)
{
    T = normalize(abs(N.z) < 0.999f ? cross(N, float3(0, 0, 1)) : cross(N, float3(0, 1, 0)));
    B = cross(N, T);
}

float3 ApplyDirectionalLighting(in ResolvedMaterial material, in float3 V, RaytracingAccelerationStructure RaytracingScene)
{    
    float3 result = float3(0.0f, 0.0f, 0.0f);
    for (uint i = 0; i < SceneConstants.NumDirectionalLightsInUse; ++i)
    {
        const DirectionalLightData dirLight = SceneConstants.DirectionalLights[i];
        const float3 L = normalize(-dirLight.Direction);
                
        RayDesc ray;
        ray.Direction = L;
        ray.TMin = 0.01f;
        ray.TMax = 1000000.0f;
    
        static const uint SamplesPerLight = 1;
        static const float SampleWeight = 1.0f / SamplesPerLight;
        
        float shadowFactor = 1.0f;
        for (uint i = 0; i < SamplesPerLight; ++i)
        {
            ray.Origin = material.WorldPosition + material.WorldNormal * 0.00001f;
            
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

float3 ApplyLighting(in ResolvedMaterial material, in float3 V)
{
    RaytracingAccelerationStructure RaytracingScene = ResourceDescriptorHeap[SceneConstants.RaytracingSceneDescriptorIndex];
    float3 result = float3(0.0f, 0.0f, 0.0f);
    result += ApplyDirectionalLighting(material, V, RaytracingScene);
    return result;
}

#endif //LIGHTING_COMMON_HLSL