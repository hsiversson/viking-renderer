#include "sceneconstants.hlsl"
#include "instancing.hlsl"
#include "pbrutils.hlsl"

cbuffer PerBatchConstantBuffer : register(b0)
{
    uint BatchInstanceDataOffsetStart;
    uint RaytracingSceneDescriptor;
};

SamplerState g_SamplerPointClamp : register(s0);
SamplerState g_SamplerBilinearClamp : register(s1);

struct PSInput
{
    float4 position : SV_POSITION;
    float3 worldPosition : WORLD_POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 uv : UV;
    uint instanceID : INSTANCE_ID;
};
struct InstanceData
{
	float4x4 LocalToWorld;
    uint MaterialID;
};

float4 MainPS(PSInput input) : SV_TARGET
{
    InstanceData data = GetInstanceData<InstanceData>(BatchInstanceDataOffsetStart, input.instanceID);
    
    MaterialParameters materialParameters = LoadMaterialParameters(data.MaterialID);
    
    float3 albedo = materialParameters.albedoTexture.Sample(g_SamplerBilinearClamp, input.uv).rgb;
    float2 compressedNormal = materialParameters.normalTexture.Sample(g_SamplerBilinearClamp, input.uv).rg;
    compressedNormal = compressedNormal * 2.0f - 1.0f; //Convert to -1,1 space
    //Reconstruct Z component of normal
    float3 detailnormal = normalize(float3(compressedNormal.x, compressedNormal.y, sqrt(1.0f - compressedNormal.x * compressedNormal.x - compressedNormal.y * compressedNormal.y)));
    //Convert normal to worldspace using the tangent frame
    float3 normal = normalize(input.normal);
    float3 tangent = normalize(input.tangent.rgb);
    float3 binormal = cross(normal, tangent) * input.tangent.w;
    float3x3 tangentToLocal = float3x3(tangent.x, binormal.x, normal.x,
                                       tangent.y, binormal.y, normal.y,
                                       tangent.z, binormal.z, normal.z);
    float3 localNormal = mul(tangentToLocal, detailnormal);
    float3 worldNormal = normalize(mul(data.LocalToWorld, float4(localNormal, 0)).xyz);
    float4 pbrParams = materialParameters.materialTexture.Sample(g_SamplerBilinearClamp, input.uv);
    float ao = pbrParams.r;
    float roughness = pbrParams.g;
    float metallic = pbrParams.b;
    
    //Lighting
    
    float3 N = worldNormal;
    float3 V = normalize(SceneConstants.CameraPosition - input.worldPosition);
    
    //Directional light lighting
    float3 L = normalize(-SceneConstants.DirectionalLightDirection);
    float3 H = normalize(V + L);
    
    float3 radiance = SceneConstants.DirectionalLightColor; //Directional light doesnt attenuate with distance
    
    float3 F0 = float3(0.04,0.04,0.04);
    F0 = lerp(F0, albedo, metallic);
    float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    
    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    float3 specular = numerator / denominator;
    
    float3 kS = F;
    float3 kD = float3(1.0,1.0,1.0) - kS;
    kD *= 1.0 - metallic;
    
    float NdotL = max(dot(N, L), 0.0);
    float3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;
    
    float3 ambient = float3(0.03,0.03,0.03) * albedo * ao;
    float3 color = ambient + Lo;
    
    //Shadowing
    
    RaytracingAccelerationStructure RaytracingScene = ResourceDescriptorHeap[RaytracingSceneDescriptor];
    
    RayDesc ray;
    ray.Origin = input.worldPosition + input.normal * 0.00001f;
    ray.Direction = L;
    ray.TMin = 0.01f;
    ray.TMax = 1000000.0f;
    
    RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> rayQuery;
    rayQuery.TraceRayInline(RaytracingScene, 0, 0xff, ray);
    rayQuery.Proceed();
    
    if (rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        color = float3(0, 0, 0);
    }
    
    return float4(color, 1.0f);
}