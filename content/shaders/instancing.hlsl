#ifndef INSTANCING_HLSL
#define INSTANCING_HLSL

#include "sceneconstants.hlsl"

template<typename T>
T GetInstanceData(uint batchInstanceDataStart, uint instanceIndex)
{
    //This way we can have a mix if instance data structures
    ByteAddressBuffer instanceDataBuffer = ResourceDescriptorHeap[SceneConstants.InstanceDataBufferDescriptorIndex];
    Buffer<uint> instanceDataOffsetBuffer = ResourceDescriptorHeap[SceneConstants.InstanceDataOffsetBufferDescriptorIndex];
    uint realInstanceDataOffset = instanceDataOffsetBuffer[batchInstanceDataStart+instanceIndex];
    T data = instanceDataBuffer.template Load<T>(realInstanceDataOffset);
    return data;
}

#endif