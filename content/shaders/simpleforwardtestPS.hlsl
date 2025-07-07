cbuffer ConstantBuffer : register(b0)
{
	float4x4 ViewProjection;
	float4x4 World;
	float3 BaseColor;
    uint TextureDescriptor;
    uint RaytracingSceneDescriptor;
    uint3 pad;
};

SamplerState g_SamplerPointClamp : register(s0);
SamplerState g_SamplerBilinearClamp : register(s1);

struct PSInput
{
    float4 position : SV_POSITION;
    float3 worldPosition : WORLD_POSITION;
    float3 normal : NORMAL;
    float2 uv : UV;
};

float4 MainPS(PSInput input) : SV_TARGET
{
    Texture2D tex = ResourceDescriptorHeap[TextureDescriptor];
    float4 texColor = tex.Sample(g_SamplerBilinearClamp, input.uv);
    
    const float3 toLightDirection = normalize(float3(-0.2f, 0.5f, -0.6f));
    float NoL = saturate(dot(input.normal, toLightDirection));
    
    RaytracingAccelerationStructure RaytracingScene = ResourceDescriptorHeap[RaytracingSceneDescriptor];
    
    RayDesc ray;
    ray.Origin = input.worldPosition + input.normal * 0.00001f;
    ray.Direction = toLightDirection;
    ray.TMin = 0.01f;
    ray.TMax = 1000000.0f;
    
    RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> rayQuery;
    rayQuery.TraceRayInline(RaytracingScene, 0, 0xff, ray);
    rayQuery.Proceed();
    
    if (rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        texColor.rgb = float3(0, 0, 0);
    }
    
    return float4(texColor.rgb * NoL, 1.0f);
}