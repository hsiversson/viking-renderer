#ifndef PBR_UTILS_HLSL
#define PBR_UTILS_HLSL

#include "materialcommon.hlsl"

bool SampleGGXSpecular(float2 xi, float3 N, float3 V, float roughness, out float3 L, out float pdf)
{
    float alpha = roughness * roughness;

    // Build tangent space for N
    float3 Up = abs(N.y) < 0.999f ? float3(0, 1, 0) : float3(1, 0, 0);
    float3 Tangent = normalize(cross(Up, N));
    float3 Bitangent = cross(N, Tangent);

    // Transform view direction to tangent space
    float3 Vt = float3(dot(V, Tangent), dot(V, Bitangent), dot(V, N));
    Vt = normalize(Vt);

    // Sample VNDF according to Heitz 2014
    float a = alpha;

    // Stretch view vector
    float3 Vh = normalize(float3(a * Vt.x, a * Vt.y, Vt.z));

    // Orthonormal basis
    float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
    float3 T1 = lensq > 0 ? float3(-Vh.y, Vh.x, 0) / sqrt(lensq) : float3(1, 0, 0);
    float3 T2 = cross(Vh, T1);

    // Sample point with polar coordinates (r, phi)
    float r = sqrt(xi.x);
    float phi = 2.0f * PI * xi.y;
    float t1 = r * cos(phi);
    float t2 = r * sin(phi);
    float s = 0.5f * (1.0f + Vh.z);
    t2 = (1.0f - s) * sqrt(1.0f - t1 * t1) + s * t2;

    // Compute normal in stretched hemisphere
    float3 Nh = t1 * T1 + t2 * T2 + sqrt(max(0.0f, 1.0f - t1 * t1 - t2 * t2)) * Vh;

    // Unstretch
    float3 Ht = normalize(float3(a * Nh.x, a * Nh.y, max(0.0f, Nh.z)));

    // Transform H back to world space
    float3 H = normalize(Tangent * Ht.x + Bitangent * Ht.y + N * Ht.z);

    // Reflect view vector about half-vector
    L = normalize(reflect(-V, H));
    if (!all(isfinite(L)))
        return false;

    // Compute PDF: D(h) * (n·h) / (4 * v·h)
    float NoH = saturate(dot(N, H));
    float VoH = saturate(dot(V, H));

    // GGX NDF
    float alpha2 = alpha * alpha;
    float denom = NoH * NoH * (alpha2 - 1.0f) + 1.0f;
    float D = alpha2 / (PI * denom * denom + FLT_EPSILON_VALUE);

    pdf = (D * NoH) / (4.0f * VoH + FLT_EPSILON_VALUE);

    return true;
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float roughnessSquared = roughness * roughness;
    float NdotH = saturate(dot(N, H));
    float NdotH2 = NdotH * NdotH;
	
    float nom = roughnessSquared;
    float denom = (NdotH2 * (roughnessSquared - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return nom / (denom + FLT_EPSILON_VALUE);
}

float GeometrySchlickGGX(float NdotV, float k)
{
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / (denom + FLT_EPSILON_VALUE);
}
  
float GeometrySmith(float3 N, float3 V, float3 L, float k)
{
    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));
    float ggx1 = GeometrySchlickGGX(NdotV, k);
    float ggx2 = GeometrySchlickGGX(NdotL, k);
	
    return ggx1 * ggx2;
}

float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float2 SampleUniformDisk(float2 xi)
{
    float r = sqrt(xi.x);
    float theta = 2.0 * PI * xi.y;
    return float2(r * cos(theta), r * sin(theta));
}

float3 SampleHemisphereCosine(float2 xi, float3 N)
{
    float phi = 2.0f * PI * xi.x;
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
    float3 F = fresnelSchlick(saturate(dot(H, V)), F0);
    float NDF = DistributionGGX(mat.WorldNormal, H, mat.Roughness);
    float G = GeometrySmith(mat.WorldNormal, V, L, mat.Roughness);
    
    float3 numerator = NDF * G * F;
    float denominator = 4.0 * NdotV * NdotL;
    float3 specular = numerator / (denominator + FLT_EPSILON_VALUE);
    
    kS = F;
    return specular;
}

float3 ComputeLuminance(in const ResolvedMaterial mat, float3 V, float3 L, float3 LightColor)
{
    float3 Result;
    
    float NdotL = saturate(dot(mat.WorldNormal, L));
    
    float3 kS;
    float3 specular = ComputeSpecularBRDF(mat, V, L, kS);
    
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= 1.0 - mat.Metallic;
    
    float3 Lo = (kD * mat.Albedo / (PI + specular + FLT_EPSILON_VALUE)) * LightColor * NdotL;
    
    float3 ambient = float3(0.03, 0.03, 0.03) * mat.Albedo * mat.AO;
    Result = ambient + Lo;
    
    return Result;
}

#endif