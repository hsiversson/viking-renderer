#ifndef INSTANCING_HLSL
#define INSTANCING_HLSL

#include "sceneconstants.hlsl"

//For now InstanceData is common. This is because the RT shader needs to access instance data from any object and we use inline raytracing. If we change to RT with subobjects we can start 
//separating the isntancedata definition. The instancebuffer at least accepts any combination of structs as its a byteaddressbuffer
struct InstanceData
{
    float4x4 LocalToWorld;
    float4x4 PrevLocalToWorld;
    uint MaterialID;
    uint IndexBufferDescriptorIndex;
    uint IndexStride;
    uint VertexBufferDescriptorIndex;
    uint VertexStride;
    uint VertexPositionByteOffset;
    uint VertexNormalByteOffset;
    uint VertexTangentByteOffset;
    uint VertexUVByteOffset;
    uint3 Pad;
};

template<typename T>
T GetInstanceData(uint batchInstanceDataStart, uint instanceIndex)
{
    //This way we can have a mix of instance data structures
    ByteAddressBuffer instanceDataBuffer = ResourceDescriptorHeap[SceneConstants.InstanceDataBufferDescriptorIndex];
    Buffer<uint> instanceDataOffsetBuffer = ResourceDescriptorHeap[SceneConstants.InstanceDataOffsetBufferDescriptorIndex];
    uint realInstanceDataOffset = instanceDataOffsetBuffer[batchInstanceDataStart+instanceIndex];
    T data = instanceDataBuffer.template Load<T>(realInstanceDataOffset);
    return data;
}

//Used for raytracing path. This will be used to index directly into the instancedata as we dont have batches for passes
template<typename T>
T GetInstanceData(uint instanceIndex)
{
    //This way we can have a mix of instance data structures
    ByteAddressBuffer instanceDataBuffer = ResourceDescriptorHeap[SceneConstants.InstanceDataBufferDescriptorIndex];
    T data = instanceDataBuffer.template Load<T>(instanceIndex);
    return data;
}

#endif