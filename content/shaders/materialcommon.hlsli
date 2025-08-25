#ifndef MATERIAL_COMMON_HLSLI
#define MATERIAL_COMMON_HLSLI

#include "common.hlsli"

struct ResolvedMaterial
{
    float3 WorldPosition;
    float3 WorldNormal;
    float3 Albedo;
    float3 Emission;
    float Roughness;
    float Metallic;
    float AO;
};

#endif //MATERIAL_COMMON_HLSL