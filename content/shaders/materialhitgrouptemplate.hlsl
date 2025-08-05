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
    
    payload.irradiance = ApplyLighting(resolvedMaterial, -WorldRayDirection());

    
    if(payload.recursionDepth < 1)
    {
        // diffuse indirect
        //{
        //    // add diffuse indirect contrib and make sure to weight properly against brdf
        //}
        
        // specular indirect
        {
            const uint2 pixel = DispatchRaysIndex().xy;

            float pdf;
            float2 xi;
            xi.x = InterleavedGradientNoise(pixel,SceneConstants.FrameIndex);
            xi.y = InterleavedGradientNoise(pixel.yx + 17,SceneConstants.FrameIndex);

            float3 reflectionDir = SampleGGXSpecular(xi, resolvedMaterial.WorldNormal, -WorldRayDirection(), resolvedMaterial.Roughness, pdf);

            RaytracingAccelerationStructure RaytracingScene = ResourceDescriptorHeap[SceneConstants.RaytracingSceneDescriptorIndex];
            RaytracingPayload specularPayload = (RaytracingPayload)0;
            specularPayload.recursionDepth = payload.recursionDepth+1;

            RayDesc ray;
            ray.Origin = resolvedMaterial.WorldPosition + resolvedMaterial.WorldNormal * 0.00001f;
            ray.Direction = reflectionDir;
            ray.TMin = 0.0f;
            ray.TMax = 1000000.0f;

            uint flags = RAY_FLAG_NONE;
            flags |= RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES;
            TraceRay(RaytracingScene, flags, 0xff, 0, 0, 0, ray, specularPayload);
            
            // add diffuse indirect contrib and make sure to weight properly against brdf
            float3 Li = specularPayload.irradiance;
            float3 kS;
            float3 L = reflectionDir;
            float NdotL = saturate(dot(resolvedMaterial.WorldNormal,L));
            float3 specularBRDF = ComputeSpecularBRDF(resolvedMaterial, -WorldRayDirection(), L, kS);
            float3 specular = specularBRDF * Li * NdotL / pdf;
            payload.irradiance += specular;
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