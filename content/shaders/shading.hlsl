#ifndef SHADING_COMMON_HLSL
#define SHADING_COMMON_HLSL

#include "common.hlsl"
#include "bxdf.hlsl"
#include "materialcommon.hlsl"

// Default shading model
float3 ApplyShading(in ResolvedMaterial material, in float3 V, in float3 L)
{
    float3 H = normalize(V + L);
    
    float NdotL = saturate(dot(material.worldNormal, L));
    float NdotV = saturate(dot(material.worldNormal, V));
    float HdotV = saturate(dot(H, V));
    
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), material.color, material.metallic);
    
    float3 F = F_Schlick(HdotV, F0);
    float NDF = D_GGX(material.worldNormal, H, material.roughness);
    float G = G_Smith(material.worldNormal, V, L, material.roughness);
    
    float3 kS = F;
    float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
    kD *= 1.0f - material.metallic;
    
    float3 specular = (NDF * G * F) / max(4.0 * NdotV * NdotL, 0.00001f);
    float3 diffuse = kD * material.color / PI;
    
    return (diffuse + specular) * NdotL;
}

#endif //LIGHTING_COMMON_HLSL