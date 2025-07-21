#ifndef SCENECONSTANTS_HLSL
#define SCENECONSTANTS_HLSL

struct DirectionalLightData
{
    float3 Emission;
    float Radius;    
    float3 Direction;
    float _pad;
};

struct SceneConstantsStruct
{
    float4x4 View;
    float4x4 InvView;
    float4x4 Projection;
    float4x4 InvProjection;
    float4x4 ViewProjection;
    float4x4 InvViewProjection;
    float4x4 ViewProjectionNoJitter;
    uint InstanceDataBufferDescriptorIndex;         // Descriptor index to the global buffer where all instance data for the scene is stored
    uint InstanceDataOffsetBufferDescriptorIndex;   // Descriptor index to the buffer which contains indices for this pass where the instance data of every instance of the batch is stored
    uint MaterialDataBufferDescriptorIndex;         // Descriptor index to the global buffer where all material data for the scene is stored
    uint pad0;
    float3 CameraPosition;
    uint NumDirectionalLightsInUse;
    DirectionalLightData DirectionalLights[2];
};

ConstantBuffer<SceneConstantsStruct> SceneConstants : register(b0, space1);

#endif
