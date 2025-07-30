#ifndef PBR_UTILS_HLSL
#define PBR_UTILS_HLSL

#define PI 3.1415926535

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

struct PBRMaterialInput
{
    float3 WorldPosition;
    float3 WorldNormal;
    float3 Albedo;
    float Roughness;
    float Metallic;
    float AO;
};

float3 ComputeLuminance(in const PBRMaterialInput mat, float3 CameraWorld, float3 L, float3 LightEmission)
{
    float3 Result;
    
    float3 V = normalize(CameraWorld - mat.WorldPosition);
    const float3 H = normalize(V + L);
    float NdotL = max(dot(mat.WorldNormal, L), 0.0);
    float NdotV = max(dot(mat.WorldNormal, V), 0.0);
    
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, mat.Albedo, mat.Metallic);
    float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    float NDF = DistributionGGX(mat.WorldNormal, H, mat.Roughness);
    float G = GeometrySmith(mat.WorldNormal, V, L, mat.Roughness);
    
    float3 numerator = NDF * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.0001;
    float3 specular = numerator / denominator;
    
    float3 kS = F;
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= 1.0 - mat.Metallic;
    
    float3 Lo = (kD * mat.Albedo / PI + specular) * LightEmission * NdotL;
    
    float3 ambient = float3(0.03, 0.03, 0.03) * mat.Albedo * mat.AO;
    Result = ambient + Lo;
    
    return Result;
}

#endif