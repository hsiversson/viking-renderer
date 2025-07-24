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
    float4 currPosition : CURR_CLIP;
    float4 prevPosition : PREV_CLIP;
    float3 worldPosition : WORLD_POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 uv : UV;
    uint instanceID : INSTANCE_ID;
};

struct PSOutput
{
    float4 Color : SV_Target0;
    float2 Velocity : SV_Target1;
};

struct InstanceData
{
	float4x4 LocalToWorld;
    float4x4 PrevLocalToWorld;
    uint MaterialID;
};

float2 CalcVelocity(float4 newPos, float4 oldPos)
{
    float2 prevPos = oldPos.xy / oldPos.w;
    float2 currPos = newPos.xy / newPos.w;
    
    currPos -= SceneConstants.CurrentJitter;
    prevPos -= SceneConstants.PrevJitter;
    
    currPos = currPos * float2(0.5f, -0.5f) + 0.5f;
    prevPos = prevPos * float2(0.5f, -0.5f) + 0.5f;
    
    float2 velocity = prevPos - currPos; // Really were computing inverse motion vector here, so later we need to add it
    
    return velocity;
}

PSOutput MainPS(PSInput input)
{
    InstanceData data = GetInstanceData<InstanceData>(BatchInstanceDataOffsetStart, input.instanceID);
    
    MaterialParameters materialParameters = LoadMaterialParameters(data.MaterialID);
    
    float3 albedo = materialParameters.albedoTexture.Sample(g_SamplerBilinearClamp, input.uv).rgb;
    float2 compressedNormal = materialParameters.normalTexture.Sample(g_SamplerBilinearClamp, input.uv).rg;
    compressedNormal = compressedNormal * 2.0f - 1.0f; //Convert to -1,1 space
    //Reconstruct Z component of normal
    float3 detailnormal = normalize(float3(compressedNormal.x, compressedNormal.y, sqrt(1.0f - compressedNormal.x * compressedNormal.x - compressedNormal.y * compressedNormal.y)));
    detailnormal.y = -detailnormal.y;
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
    
    float3 V = normalize(SceneConstants.CameraPosition - input.worldPosition);
    float3 N = worldNormal;
    
    RaytracingAccelerationStructure RaytracingScene = ResourceDescriptorHeap[RaytracingSceneDescriptor];
    float3 lightingResult = float3(0.0, 0.0, 0.0);
    //Lighting
    
    //Directional light lighting
    for (uint i = 0; i < SceneConstants.NumDirectionalLightsInUse; ++i)
    {
        const DirectionalLightData dirLight = SceneConstants.DirectionalLights[i];
        const float3 L = normalize(-dirLight.Direction);
        const float3 H = normalize(V + L);
        
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
            continue; // we hit geometry, this means we're in shadow for this light
        }
        
        float3 F0 = float3(0.04, 0.04, 0.04);
        F0 = lerp(F0, albedo, metallic);
        float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
    
        float3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        float3 specular = numerator / denominator;
    
        float3 kS = F;
        float3 kD = float3(1.0, 1.0, 1.0) - kS;
        kD *= 1.0 - metallic;
    
        float NdotL = max(dot(N, L), 0.0);
        float3 Lo = (kD * albedo / PI + specular) * dirLight.Emission * NdotL;
    
        float3 ambient = float3(0.03, 0.03, 0.03) * albedo * ao;
        lightingResult += ambient + Lo;
    }
    
    PSOutput output;
    output.Color = float4(lightingResult, 1.0f);
    output.Velocity = CalcVelocity(input.currPosition, input.prevPosition);

    return output;
}