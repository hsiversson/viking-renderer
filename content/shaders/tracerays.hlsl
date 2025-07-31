#include "sceneconstants.hlsl"
#include "instancing.hlsl"
#include "pbrutils.hlsl"

struct ConstantsStruct
{
    uint SceneTextureDescriptorIndex;
    uint DepthBufferDescriptorIndex;
    uint RaytracingSceneDescriptorIndex;
};
ConstantBuffer<ConstantsStruct> Constants : register(b0);

SamplerState g_SamplerPointClamp : register(s0);
SamplerState g_SamplerBilinearClamp : register(s1);

struct TraceHitResult
{
    bool Hit;
    float3 WorldPos;
    float3 Normal;
    float4 Color;
};

struct RTVertex
{
    float3 Position;
    float3 Normal;
    float4 Tangent;
    float2 UV;
};

//======================================================================================
//==========TODO: This is a massive hack and only would work cause we only have one type of material.
//==========This is hardcoded here for now. Remove when we switch to non inlined RT

struct MaterialParameters
{
    Texture2D albedoTexture;
    Texture2D normalTexture;
    Texture2D materialTexture;
    Texture2D emissiveTexture;
};

struct PackedMaterialParameters
{
    uint albedoTexture;
    uint normalTexture;
    uint materialTexture;
    uint emissiveTexture;
};

MaterialParameters LoadMaterialParameters(uint offset)
{
    ByteAddressBuffer materialDataBuffer = ResourceDescriptorHeap[SceneConstants.MaterialDataBufferDescriptorIndex];
    PackedMaterialParameters packedParams = materialDataBuffer.Load < PackedMaterialParameters > (offset);
    MaterialParameters result;
    result.albedoTexture = ResourceDescriptorHeap[NonUniformResourceIndex(packedParams.albedoTexture)];
    result.normalTexture = ResourceDescriptorHeap[NonUniformResourceIndex(packedParams.normalTexture)];
    result.materialTexture = ResourceDescriptorHeap[NonUniformResourceIndex(packedParams.materialTexture)];
    result.emissiveTexture = ResourceDescriptorHeap[NonUniformResourceIndex(packedParams.emissiveTexture)];
    return result;
}
//===========================================================================================
//===========================================================================================


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

uint RetrieveIndex(ByteAddressBuffer IB, uint startByte, uint indexStride)
{
    //We assume the index stride is never greater than a dword
    uint alignedDword = IB.Load<uint>((startByte / 4U) * 4U);
    //GPUs are little endian
    uint shift = (startByte % 4U) * 8U; //((4 - indexStride) - (startByte % 4)) * 8;
    uint Mask = 0xFFFFFFFFU >> ((4U - indexStride) * 8U);
    uint Index = (alignedDword >> shift) & Mask;
    return Index;
}

uint3 RetrieveIndices(in const InstanceData data, uint primitiveIndex)
{
    ByteAddressBuffer IB = ResourceDescriptorHeap[data.IndexBufferDescriptorIndex];
    uint startByte = primitiveIndex * 3 * data.IndexStride;
    //Depending on the stride indices will be dword aligned or not
    uint3 Result;
    Result.x = RetrieveIndex(IB, startByte, data.IndexStride);
    Result.y = RetrieveIndex(IB, startByte + data.IndexStride, data.IndexStride);
    Result.z = RetrieveIndex(IB, startByte + 2 * data.IndexStride, data.IndexStride);
    return Result;
}

RTVertex GetInterpolatedVertexAttributes(in const InstanceData data, in const uint3 indices, in const float2 barycentrics)
{
    ByteAddressBuffer VB = ResourceDescriptorHeap[data.VertexBufferDescriptorIndex];
    
    RTVertex vertices[3];
    
    [unroll(3)]
    for (uint i = 0; i < 3; i++)
    {
        uint vertexByteOffset = indices[i] * data.VertexStride;
        vertices[i].Position = VB.Load<float3>(vertexByteOffset + data.VertexPositionByteOffset);
        vertices[i].Normal = VB.Load<float3>(vertexByteOffset + data.VertexNormalByteOffset);
        vertices[i].Tangent = VB.Load<float4>(vertexByteOffset + data.VertexTangentByteOffset);
        vertices[i].UV = VB.Load<float2>(vertexByteOffset + data.VertexUVByteOffset);
    }
    
    RTVertex interpolated;
    interpolated.Position = barycentrics.x * vertices[1].Position + barycentrics.y * vertices[2].Position + (1.0 - barycentrics.x - barycentrics.y) * vertices[0].Position;
    interpolated.Normal = barycentrics.x * vertices[1].Normal + barycentrics.y * vertices[2].Normal + (1.0 - barycentrics.x - barycentrics.y) * vertices[0].Normal;
    interpolated.Tangent = barycentrics.x * vertices[1].Tangent + barycentrics.y * vertices[2].Tangent + (1.0 - barycentrics.x - barycentrics.y) * vertices[0].Tangent;
    interpolated.UV = barycentrics.x * vertices[1].UV + barycentrics.y * vertices[2].UV + (1.0 - barycentrics.x - barycentrics.y) * vertices[0].UV;
    return interpolated;
}

TraceHitResult TraceRadianceRay(RaytracingAccelerationStructure raytracingScene, float3 rayOrigin, float3 rayDirection)
{
    TraceHitResult result;
    result.Hit = false;
    
    RayDesc ray;
    ray.Origin = rayOrigin;
    ray.Direction = rayDirection; //toLight
    ray.TMin = 0.01f; //offset
    ray.TMax = 1000000.0f; //distanceToLight
    
    RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> rayQuery;
    rayQuery.TraceRayInline(raytracingScene, 0, 0xff, ray);
    rayQuery.Proceed();
    
    if (rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        result.Hit = true;
        uint instanceID = rayQuery.CommittedInstanceID();
        uint primitiveIndex = rayQuery.CommittedPrimitiveIndex();
        float2 bary = rayQuery.CommittedTriangleBarycentrics();
        float t = rayQuery.CommittedRayT();
        
        InstanceData data = GetInstanceData < InstanceData > (instanceID);
        
        //Gather geometry properties from barycentric coordinates
        uint3 indices = RetrieveIndices(data, primitiveIndex);
        RTVertex vertex = GetInterpolatedVertexAttributes(data, indices, bary);
        float3 WorldPos = mul(data.LocalToWorld, float4(vertex.Position, 1.0f)).xyz;
        
        //From interpolated values calculate final PS input structure
        
        //Gather surface properties using MaterialID
        MaterialParameters mat = LoadMaterialParameters(data.MaterialID);
        
        //For now just sample basic properties from textures in material. We need to find a way to have different materials fill in the PBR properties in custom ways
        PBRMaterialInput pbrInput;
        pbrInput.WorldPosition = WorldPos;
        
        
        //Compute tangent frame
        float3 normal = normalize(vertex.Normal);
        float3 tangent = normalize(vertex.Tangent.rgb);
        float3 binormal = cross(normal, tangent) * vertex.Tangent.w;
        float3x3 tangentToLocal = float3x3(tangent.x, binormal.x, normal.x,
                                       tangent.y, binormal.y, normal.y,
                                       tangent.z, binormal.z, normal.z);
        
        float2 compressedNormal = mat.normalTexture.Sample(g_SamplerBilinearClamp, vertex.UV).rg;
        compressedNormal = compressedNormal * 2.0f - 1.0f; //Convert to -1,1 space
        //Reconstruct Z component of normal
        float3 detailnormal = normalize(float3(compressedNormal.x, compressedNormal.y, sqrt(1.0f - compressedNormal.x * compressedNormal.x - compressedNormal.y * compressedNormal.y)));
        detailnormal.y = -detailnormal.y;
        float3 localNormal = mul(tangentToLocal, detailnormal);
        pbrInput.WorldNormal = normalize(mul(data.LocalToWorld, float4(localNormal, 0)).xyz);
        
        pbrInput.Albedo = mat.albedoTexture.Sample(g_SamplerBilinearClamp, vertex.UV).rgb;;
        
        float4 pbrParams = mat.materialTexture.Sample(g_SamplerBilinearClamp, vertex.UV);
        pbrInput.AO = pbrParams.r;
        pbrInput.Roughness = pbrParams.g;
        pbrInput.Metallic = pbrParams.b;
        
        //Evaluate direct brdf based on 
        float3 lightingResult = float3(0.0f, 0.0f, 0.0f);
        for (uint i = 0; i < SceneConstants.NumDirectionalLightsInUse; ++i)
        {
            const DirectionalLightData dirLight = SceneConstants.DirectionalLights[i];
            const float3 L = normalize(-dirLight.Direction);
            
            RayDesc ray;
            ray.Origin = pbrInput.WorldPosition + pbrInput.WorldNormal * 0.00001f;
            ray.Direction = L;
            ray.TMin = 0.01f;
            ray.TMax = 1000000.0f;
            
            RayQuery < RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH > rayQuery;
            rayQuery.TraceRayInline(raytracingScene, 0, 0xff, ray);
            rayQuery.Proceed();
        
            if (rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
            {
                continue; // we hit geometry, this means we're in shadow for this light
            }
            
            lightingResult += ComputeLuminance(pbrInput, SceneConstants.CameraPosition, L, dirLight.Emission);
        }
        result.Color = float4(lightingResult, 1.0f);
    }
    
    return result;
}

[numthreads(8,8,1)]
void Main(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    RWTexture2D<float4> SceneTexture = ResourceDescriptorHeap[Constants.SceneTextureDescriptorIndex];
    
    uint width, height;
    SceneTexture.GetDimensions(width, height);
    
    float2 texelSize = 1.0f / float2(width, height);
    const uint2 pixel = DispatchThreadId.xy;
    const float2 uv = (pixel + 0.5f) * texelSize;
    float2 ndc = float2(uv.x * 2.0f - 1.0f, (1.0f - uv.y) * 2.0f - 1.0f);
    
    //Unproject pixel to get ray origin and direction
    float4 clipPos = float4(ndc, 1.0f, 1.0f);
    float4 viewPos = mul(SceneConstants.InvProjection, clipPos);
    viewPos /= viewPos.w;
    float3 rd = normalize(viewPos.xyz);
    rd = normalize(mul(SceneConstants.InvView, float4(rd, 0.0f))).xyz;
    float4 worldPos = mul(SceneConstants.InvView, float4(viewPos.xyz, 1.0f));
    float3 ro = worldPos.xyz;
    
    //Fire ray
    RaytracingAccelerationStructure raytracingScene = ResourceDescriptorHeap[Constants.RaytracingSceneDescriptorIndex];
    
    float4 resultColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    TraceHitResult primaryHit = TraceRadianceRay(raytracingScene, ro, rd);
    if (primaryHit.Hit)
    {
        resultColor = primaryHit.Color;
        // monte carlo importance sampling chooses ray dirs
        
        // diffuse indirect
        //{
        //    TraceHitResult diffuseHit = TraceRadianceRay(raytracingScene, ro, rd);
            
        //    // add diffuse indirect contrib and make sure to weight properly against brdf
        //}
        
        // specular indirect
        //{
        //    TraceHitResult specularHit = TraceRadianceRay(raytracingScene, ro, rd);
            
        //    // add diffuse indirect contrib and make sure to weight properly against brdf
        //}
    }
    else
    {
        //Sky rendering
        static const int ColorCount = 6;
        float4 sunsetColors[ColorCount] =
        {
            float4(0.05, 0.10, 0.30, 1.0), // Deep blue (dusk sky)
            float4(0.35, 0.10, 0.40, 1.0), // Violet
            float4(0.70, 0.20, 0.50, 1.0), // Magenta/pink
            float4(0.95, 0.45, 0.30, 1.0), // Orange
            float4(1.00, 0.70, 0.30, 1.0), // Golden orange
            float4(1.00, 0.85, 0.60, 1.0) // Pale yellow/gold (sun glow)
        };
        
        float3 camToPixel = ro.xyz - SceneConstants.CameraPosition;
        
        float3 toPixelDir = normalize(camToPixel);
        
        float t = 1.0 - abs(dot(toPixelDir, float3(0, 1, 0)));
        
        t = saturate(t);
        float scaled = t * (ColorCount - 1);
        int i = (int) floor(scaled);
        float localT = frac(scaled);
        i = clamp(i, 0, ColorCount - 2);
        float4 SunsetColor = lerp(sunsetColors[i], sunsetColors[i + 1], localT);
        
        // Calculate simple sun disk
        for (uint lightIdx = 0; lightIdx < SceneConstants.NumDirectionalLightsInUse; ++lightIdx)
        {
            const DirectionalLightData dirLight = SceneConstants.DirectionalLights[lightIdx];

            float sunDot = dot(toPixelDir, -normalize(dirLight.Direction)); // cos(angle)
            float cosInner = cos(dirLight.Radius); // Hard edge
            float cosOuter = cos(dirLight.Radius * 2.0f); // Feathered falloff

            float sunFactor = saturate((sunDot - cosOuter) / (cosInner - cosOuter));

            // Optionally apply power falloff for a softer edge
            sunFactor = pow(sunFactor, 4.0); // tweak this for sharpness
            
            float3 sunDiskColor = dirLight.Emission * sunFactor;
            SunsetColor.rgb += sunDiskColor;
        }
        
        resultColor = SunsetColor;
    }
    SceneTexture[pixel] = resultColor;
}