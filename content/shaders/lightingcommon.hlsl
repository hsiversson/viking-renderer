#ifndef LIGHTING_COMMON_HLSL
#define LIGHTING_COMMON_HLSL

#include "common.hlsl"
#include "sceneconstants.hlsl"
#include "shading.hlsl"

float3 ApplyLighting(in ResolvedMaterial material, in float3 V)
{
    float3 result = float3(0.0f, 0.0f, 0.0f);
    
    RaytracingAccelerationStructure RaytracingScene; // TODO
    
    for (uint i = 0; i < SceneConstants.NumDirectionalLightsInUse; ++i)
    {
        const DirectionalLightData dirLight = SceneConstants.DirectionalLights[i];
        const float3 L = normalize(-dirLight.Direction);
        
        RayDesc ray;
        ray.Origin = material.worldPosition + material.worldNormal * 0.00001f;
        ray.Direction = L;
        ray.TMin = 0.01f;
        ray.TMax = 1000000.0f;
    
        RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> rayQuery;
        rayQuery.TraceRayInline(RaytracingScene, 0, 0xff, ray);
        rayQuery.Proceed();
        
        if (rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
        {
            continue; // we hit geometry, this means we're in shadow for this light
        }
        
        result += ApplyShading(material, V, L) * dirLight.Emission;
    }
    
    return result;
}

#endif //LIGHTING_COMMON_HLSL