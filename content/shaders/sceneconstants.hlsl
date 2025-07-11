#ifndef SCENECONSTANTS_HLSL
#define SCENECONSTANTS_HLSL

cbuffer PerFrameConstantBuffer : register(b0, space1)
{
    float4x4 WorldToClip;
    uint InstanceDataBufferDescriptorIndex; // Descriptor index to the global buffer where all instance data for the scene is stored
    uint InstanceDataOffsetBufferDescriptorIndex; // Descriptor index to the bufeer which contains indiced for this pass where the instance data of every instance of the batch is stored
    float3 CameraPosition;
    float3 DirectionalLightDirection;
    float3 DirectionalLightColor;
};

#endif
