#include "lightingcommon.hlsl"
#include "pbrutils.hlsl"
#include "raytracingcommon.hlsl"

SamplerState g_SamplerPointClamp : register(s0);
SamplerState g_SamplerBilinearClamp : register(s1);

struct InstanceData
{
$INSTANCE_DATA$
};

InstanceData LoadInstanceData(in uint instanceIndex)
{
    ByteAddressBuffer instanceDataBuffer = ResourceDescriptorHeap[SceneConstants.InstanceDataBufferDescriptorIndex];
    InstanceData data = instanceDataBuffer.Load<InstanceData>(instanceIndex);
    return data;
}

struct MaterialParameters
{
$MATERIAL_PARAMETERS$
};

struct PackedMaterialParameters
{
$PACKED_MATERIAL_PARAMETERS$
};

MaterialParameters LoadMaterialParameters(in uint offset)
{
    ByteAddressBuffer materialDataBuffer = ResourceDescriptorHeap[SceneConstants.MaterialDataBufferDescriptorIndex];
    PackedMaterialParameters packedParams = materialDataBuffer.Load<PackedMaterialParameters>(offset);
    MaterialParameters resolvedMaterialParams;
    
$RESOLVE_MATERIAL_PARAMETERS$
    
    return resolvedMaterialParams;
}

struct ResolvedHitInfo
{
$RESOLVED_HIT_INFO$
};

ResolvedHitInfo ResolveHit(in InstanceData instanceData, in uint3 indices, in float3 barycentrics)
{
    ResolvedHitInfo resolvedHitInfo;
    
$RESOLVE_HIT$
    
    return resolvedHitInfo;
}

ResolvedMaterial ResolveMaterial(in InstanceData instanceData, in ResolvedHitInfo hitInfo, in MaterialParameters materialParameters)
{
    ResolvedMaterial resolvedMaterial;
    
$RESOLVE_MATERIAL$
    
    return resolvedMaterial;
}

#if defined(HAS_CLOSEST_HIT)
[shader("closesthit")]
void $CLOSESTHIT_IDENTIFIER$(inout RaytracingPayload payload, in BuiltInTriangleIntersectionAttributes intersectionAttributes)
{
    const InstanceData instanceData = LoadInstanceData(InstanceID());
    
    ByteAddressBuffer indexBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(instanceData.indexBufferDescriptorIndex)];
    const uint3 indices = UnpackTriangleIndices(indexBuffer, instanceData.indexStride, PrimitiveIndex());
    
    const float3 barycentrics = ConvertBarycentricsFromCommitted(intersectionAttributes.barycentrics);
    
    const ResolvedHitInfo hitInfo = ResolveHit(instanceData, indices, barycentrics);
    
    const MaterialParameters materialParameters = LoadMaterialParameters(instanceData.materialId);
    ResolvedMaterial resolvedMaterial = ResolveMaterial(instanceData, hitInfo, materialParameters);
    resolvedMaterial.Roughness = max(resolvedMaterial.Roughness, 0.05f); // Clamp roughness such that ggx evals doesn't explode.
    
    payload.worldNormal = resolvedMaterial.WorldNormal;
    payload.irradiance = ApplyLighting(resolvedMaterial, -WorldRayDirection()) + resolvedMaterial.Emission;
    
    if(payload.recursionDepth < 1)
    {
        const uint2 pixel = DispatchRaysIndex().xy;
        RaytracingAccelerationStructure RaytracingScene = ResourceDescriptorHeap[SceneConstants.RaytracingSceneDescriptorIndex];
        static const uint NumIndirectRays = 1;
        static const float RayWeight = 1.0f / NumIndirectRays;
        
        // diffuse indirect
        for (uint rayIdx = 0; rayIdx < NumIndirectRays; ++rayIdx)
        {
            float2 xi;
            xi.x = RandomFloat01(payload.rngState);
            xi.y = RandomFloat01(payload.rngState);
            
            float3 diffuseDir = SampleHemisphereCosine(xi, resolvedMaterial.WorldNormal); // tangent-space cosine-weighted
            
            RayDesc ray;
            ray.Origin = resolvedMaterial.WorldPosition + resolvedMaterial.WorldNormal * FLT_SMALL_VALUE;
            ray.Direction = diffuseDir;
            ray.TMin = 0.0f;
            ray.TMax = 1000000.0f;

            uint flags = RAY_FLAG_NONE;
            flags |= RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES;
            
            RaytracingPayload diffusePayload = (RaytracingPayload) 0;
            diffusePayload.recursionDepth = payload.recursionDepth + 1;
            diffusePayload.rngState = payload.rngState;
            TraceRay(RaytracingScene, flags, 0xff, 0, 0, 0, ray, diffusePayload);
            
            float3 diffuseContribution = resolvedMaterial.Albedo * diffusePayload.irradiance;
            payload.irradiance += diffuseContribution * RayWeight;
            payload.rngState = diffusePayload.rngState;
        }
        
        // specular indirect
        for (uint rayIdx = 0; rayIdx < NumIndirectRays; ++rayIdx)
        {
            float3 L = float3(0,0,0);
            float pdf = 1.0f;
            if (resolvedMaterial.Roughness < 0.1f)
            {
                L = normalize(reflect(WorldRayDirection(), resolvedMaterial.WorldNormal));
                pdf = 1.0f;
            }
            else
            {
                float2 xi;
                xi.x = RandomFloat01(payload.rngState);
                xi.y = RandomFloat01(payload.rngState);
                
                if (!SampleGGXSpecular(xi, resolvedMaterial.WorldNormal, -WorldRayDirection(), resolvedMaterial.Roughness, L, pdf))
                    continue;
            }
            
            RaytracingPayload specularPayload = (RaytracingPayload)0;
            specularPayload.recursionDepth = payload.recursionDepth + 1;
            specularPayload.rngState = payload.rngState;

            RayDesc ray;
            ray.Origin = resolvedMaterial.WorldPosition + resolvedMaterial.WorldNormal * FLT_SMALL_VALUE;
            ray.Direction = L;
            ray.TMin = 0.0f;
            ray.TMax = 1000000.0f;

            uint flags = RAY_FLAG_NONE;
            flags |= RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES;
            TraceRay(RaytracingScene, flags, 0xff, 0, 0, 0, ray, specularPayload);
            
            float NdotL = saturate(dot(resolvedMaterial.WorldNormal,L));
            float3 Li = specularPayload.irradiance;
            float3 specular;
            if (pdf == 1.0f)
            {
                float3 F0 = lerp(float3(0.04, 0.04, 0.04), resolvedMaterial.Albedo, resolvedMaterial.Metallic);
                float3 F = fresnelSchlick(saturate(dot(L,resolvedMaterial.WorldNormal)), F0);
                specular = Li * F;
            }
            else
            {
                float3 kS;
                float3 specularBRDF = ComputeSpecularBRDF(resolvedMaterial, -WorldRayDirection(), L, kS);
                specular = specularBRDF * Li * NdotL / pdf;
            }
            payload.irradiance += specular * RayWeight;
            payload.rngState = specularPayload.rngState;
        }
    }
}
#endif //HAS_CLOSEST_HIT

#if defined(HAS_ANY_HIT)
[shader("anyhit")]
void $ANYHIT_IDENTIFIER$(inout RaytracingPayload payload, in BuiltInTriangleIntersectionAttributes intersectionAttributes)
{
    payload.irradiance = float3(0, 1, 1);
}
#endif //HAS_ANY_HIT