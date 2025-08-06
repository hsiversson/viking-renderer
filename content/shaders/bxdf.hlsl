#ifndef BXDF_COMMON_HLSL
#define BXDF_COMMON_HLSL

#include "common.hlsl"

float D_GGX(float3 N, float3 H, float roughness)
{
    float roughnessSquared = roughness * roughness;
    float NdotH = saturate(dot(N, H));
    float NdotH2 = NdotH * NdotH;
	
    float nom = roughnessSquared;
    float denom = (NdotH2 * (roughnessSquared - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return nom / (denom + FLT_EPSILON_VALUE);
}

float G_SchlickGGX(float NdotV, float k)
{
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return nom / (denom + FLT_EPSILON_VALUE);
}
  
float G_Smith(float3 N, float3 V, float3 L, float k)
{
    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));
    float ggx1 = G_SchlickGGX(NdotV, k);
    float ggx2 = G_SchlickGGX(NdotL, k);
	
    return ggx1 * ggx2;
}

float3 F_Schlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

#endif //BXDF_COMMON_HLSL