#ifndef MATERIAL_COMMON_H
#define MATEIRAL_COMMON_H

#include "common.hlsl"

struct ResolvedMaterial
{
    float3 worldPosition;
    float3 color;
    float3 worldNormal;
    float roughness;
    float metallic;
};

#endif //MATERIAL_COMMON_H