struct ConstantsStruct
{
    float4x4 ViewProjection;
    float4x4 ObjectTransform;
    uint ObjectIdLowPart;
    uint ObjectIdHighPart;
    uint VertexBufferDescriptorIndex;
    uint VertexPositionByteOffset;
    uint VertexStride;
    uint3 _pad;
};
ConstantBuffer<ConstantsStruct> Constants : register(b0);

struct PixelInput
{
    float4 clipPosition : SV_POSITION;
};

PixelInput MainVS(uint vertexId : SV_VertexID)
{
    ByteAddressBuffer vertexBuffer = ResourceDescriptorHeap[Constants.VertexBufferDescriptorIndex];
    const uint vertexOffset = vertexId * Constants.VertexStride;
    float3 localPosition = asfloat(vertexBuffer.Load3(vertexOffset + Constants.VertexPositionByteOffset));
    float3 worldPosition = mul(Constants.ObjectTransform, float4(localPosition, 1.0f)).xyz;
    
    PixelInput output;
    output.clipPosition = mul(Constants.ViewProjection, float4(worldPosition, 1.0f));
    return output;
}

uint2 MainPS(PixelInput input) : SV_Target
{
    return uint2(Constants.ObjectIdLowPart, Constants.ObjectIdHighPart);
}
