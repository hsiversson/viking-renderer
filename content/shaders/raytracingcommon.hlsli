#ifndef RAYTRACING_COMMON_HLSLI
#define RAYTRACING_COMMON_HLSLI

#include "random.hlsli"

uint UnpackIndex(ByteAddressBuffer indexBuffer, uint offset, uint stride)
{
    //We assume the index stride is never greater than a 4 bytes
    uint aligned = indexBuffer.Load<uint>((offset / 4u) * 4u);
    
    //GPUs are little endian
    uint shift = (offset % 4u) * 8u; //((4 - stride) - (offset % 4)) * 8;
    uint mask = 0xFFFFFFFFu >> ((4u - stride) * 8u);
    uint index = (aligned >> shift) & mask;
    return index;
}

uint3 UnpackTriangleIndices(ByteAddressBuffer indexBuffer, in uint stride, uint primitiveIndex)
{
    uint startByte = primitiveIndex * 3 * stride;
    
    //Depending on the stride indices will be 4 byte aligned or not
    uint3 result;
    result.x = UnpackIndex(indexBuffer, startByte, stride);
    result.y = UnpackIndex(indexBuffer, startByte + stride, stride);
    result.z = UnpackIndex(indexBuffer, startByte + 2 * stride, stride);
    return result;
}

float BarycentricLerp(in float v0, in float v1, in float v2, in float3 barycentrics)
{
    return v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;
}

float2 BarycentricLerp(in float2 v0, in float2 v1, in float2 v2, in float3 barycentrics)
{
    return v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;
}

float3 BarycentricLerp(in float3 v0, in float3 v1, in float3 v2, in float3 barycentrics)
{
    return v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;
}

float4 BarycentricLerp(in float4 v0, in float4 v1, in float4 v2, in float3 barycentrics)
{
    return v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;
}

float3 ConvertBarycentricsFromCommitted(in float2 committedBarycentrics)
{
    return float3(1.0f - committedBarycentrics.x - committedBarycentrics.y, committedBarycentrics.x, committedBarycentrics.y);
}

struct [raypayload] RaytracingPayload
{
    float3 irradiance : read(caller, closesthit, miss) : write(caller, closesthit, miss);
    float3 worldNormal : read(caller, closesthit, miss) : write(caller, closesthit, miss);
    float roughness : read(caller, closesthit, miss) : write(caller, closesthit, miss);
    float3 diffuseAlbedo : read(caller, closesthit, miss) : write(caller, closesthit, miss);
    float3 specularAlbedo : read(caller, closesthit, miss) : write(caller, closesthit, miss);
    uint recursionDepth : read(caller, closesthit, miss) : write(caller, closesthit, miss);
    uint rngState : read(caller, closesthit, miss) : write(caller, closesthit, miss);
};

#endif //RAYTRACING_COMMON_HLSL