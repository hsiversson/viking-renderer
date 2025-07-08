struct ConstantStruct
{
    float4x4 WorldToClip;
    float4x4 ClipToWorld;
    
    float4x4 WorldToCamera;
    float4x4 CameraToWorld;
    
    float4x4 CameraToClip;
    float4x4 ClipToCamera;
    
    float3 CameraPosition;
    uint DepthBufferDescriptorIndex;
    
    uint TargetTextureDescriptorIndex;
    uint RaytracingSceneDescriptorIndex;
};
ConstantBuffer<ConstantsStruct> Constants : register(b0);

float GetShadowFactor(RaytracingAccelerationStructure raytracingScene, float3 position, float3 toLight, float distanceToLight, float offset = 0.01f)
{
    RayDesc ray;
    ray.Origin = position;
    ray.Direction = toLight;
    ray.TMin = offset;
    ray.TMax = distanceToLight;
    
    RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> rayQuery;
    rayQuery.TraceRayInline(raytracingScene, 0, 0xff, ray);
    rayQuery.Proceed();
    
    return (rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT) ? 0.0f : 1.0f;
}

struct TraceHitResult
{
    bool hit;
};

TraceHitResult TraceRadianceRay(RaytracingAccelerationStructure raytracingScene, float3 rayOrigin, float3 rayDirection)
{
    TraceHitResult result;
    
    RayDesc ray;
    ray.Origin = position;
    ray.Direction = toLight;
    ray.TMin = offset;
    ray.TMax = distanceToLight;
    
    RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> rayQuery;
    rayQuery.TraceRayInline(raytracingScene, 0, 0xff, ray);
    rayQuery.Proceed();
    
    if (rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        // gather surface properties
        // evaluate direct brdf
    }
    
    return result;
}

[numthreads(8,8,1)]
void Main(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    const uint2 pixel = DispatchThreadId.xy;
    
    float3 ro;
    float3 rd;
    
    RaytracingAccelerationStructure raytracingScene = ResourceDescriptorIndex[Constants.RaytracingSceneDescriptorIndex];
    
    float4 resultColor;
    TraceHitResult primaryHit = TraceRadianceRay(raytracingScene, ro, rd);
    if (primaryHit.hit)
    {
        // monte carlo importance sampling chooses ray dirs
        
        // diffuse indirect
        {
            TraceHitResult diffuseHit = TraceRadianceRay(raytracingScene, ro, rd);
            
            // add diffuse indirect contrib and make sure to weight properly against brdf
        }
        
        // specular indirect
        {
            TraceHitResult specularHit = TraceRadianceRay(raytracingScene, ro, rd);
            
            // add diffuse indirect contrib and make sure to weight properly against brdf
        }
    }

    RWTexture2D<float4> targetTexture = ResourceDescriptorIndex[Constants.TargetTextureDescriptorIndex];
    targetTexture[pixel] = resultColor;
}