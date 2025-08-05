#ifndef MATERIAL_COMMON_HLSL
#define MATERIAL_COMMON_HLSL

#include "common.hlsl"

struct ResolvedMaterial
{
    float3 WorldPosition;
    float3 WorldNormal;
    float3 Albedo;
    float Roughness;
    float Metallic;
    float AO;
};

#endif //MATERIAL_COMMON_HLSL