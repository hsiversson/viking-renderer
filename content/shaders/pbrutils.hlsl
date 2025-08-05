#ifndef PBR_UTILS_HLSL
#define PBR_UTILS_HLSL

#include "materialcommon.hlsl"

#define PI 3.1415926535

float3 SampleGGXSpecular(float2 xi, float3 N, float3 V, float roughness, out float pdf)
{
    float alpha = roughness * roughness;

    // GGX importance sampling of the half-vector
    float phi = 2.0f * 3.14159265f * xi.x;
    float cosTheta = sqrt((1.0f - xi.y) / (1.0f + (alpha * alpha - 1.0f) * xi.y));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    // Half vector in tangent space
    float3 Ht = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    // Transform to world space (build TBN from N)
    float3 Up = abs(N.y) < 0.999f ? float3(0, 1, 0) : float3(1, 0, 0);
    float3 Tangent = normalize(cross(Up, N));
    float3 Bitangent = cross(N, Tangent);
    float3 H = normalize(Tangent * Ht.x + Bitangent * Ht.y + N * Ht.z);

    // Reflection direction
    float3 L = reflect(-V, H);

    // Compute PDF: D(h) * (n·h) / (4 * v·h)
    float NoH = saturate(dot(N, H));
    float VoH = saturate(dot(V, H));

    // GGX normal distribution function
    float alpha2 = alpha * alpha;
    float denom = NoH * NoH * (alpha2 - 1.0f) + 1.0f;
    float D = alpha2 / (3.14159265f * denom * denom);

    pdf = (D * NoH) / (4.0f * VoH + 1e-5f);

    return normalize(L);
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float roughnessSquared = roughness * roughness;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
	
    float nom = roughnessSquared;
    float denom = (NdotH2 * (roughnessSquared - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float k)
{
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return nom / denom;
}
  
float GeometrySmith(float3 N, float3 V, float3 L, float k)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, k);
    float ggx2 = GeometrySchlickGGX(NdotL, k);
	
    return ggx1 * ggx2;
}

float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float3 SampleHemisphereCosine(float2 xi, float3 N)
{
    float phi = 2.0f * 3.14159265f * xi.x;
    float cosTheta = sqrt(1.0f - xi.y);
    float sinTheta = sqrt(xi.y);

    float3 localDir = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    // Build tangent space
    float3 T = normalize(abs(N.y) < 0.999f ? cross(N, float3(0, 1, 0)) : cross(N, float3(1, 0, 0)));
    float3 B = cross(T, N);

    return localDir.x * T + localDir.y * B + localDir.z * N;
}

float3 ComputeSpecularBRDF(in const ResolvedMaterial mat, float3 V, float3 L, out float3 kS)
{
    const float3 H = normalize(V + L);
    float NdotL = saturate(dot(mat.WorldNormal, L));
    float NdotV = saturate(dot(mat.WorldNormal, V));
    
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, mat.Albedo, mat.Metallic);
    float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    float NDF = DistributionGGX(mat.WorldNormal, H, mat.Roughness);
    float G = GeometrySmith(mat.WorldNormal, V, L, mat.Roughness);
    
    float3 numerator = NDF * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.0001;
    float3 specular = numerator / denominator;
    
    kS = F;
    return specular;
}

float3 ComputeLuminance(in const ResolvedMaterial mat, float3 V, float3 L, float3 LightColor)
{
    float3 Result;
    
    float NdotL = max(dot(mat.WorldNormal, L), 0.0);
    
    float3 kS;
    float3 specular = ComputeSpecularBRDF(mat, V, L, kS);
    
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= 1.0 - mat.Metallic;
    
    float3 Lo = (kD * mat.Albedo / PI + specular) * LightColor * NdotL;
    
    float3 ambient = float3(0.03, 0.03, 0.03) * mat.Albedo * mat.AO;
    Result = ambient + Lo;
    
    return Result;
}

#endif