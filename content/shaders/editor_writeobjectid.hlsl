struct ConstantsStruct
{
    float4x4 ViewProjection;
    uint ObjectId;
    uint VertexBufferDescriptorIndex;
    uint VertexPositionByteOffset;
};
ConstantBuffer<ConstantsStruct> Constants : register(b0);

struct PixelInput
{
    float4 clipPosition : SV_POSITION;
};

PixelInput MainVS(uint vertexId : SV_VertexID)
{
    ByteAddressBuffer vertexBuffer = ResourceDescriptorHeap[Constants.VertexBufferDescriptorIndex];
    float3 position = asfloat(vertexBuffer.Load3(vertexId + Constants.VertexPositionByteOffset));
    
    PixelInput output;
    output.clipPosition = mul(Constants.ViewProjection, float4(position.xyz, 1.0f));
    return output;
}

uint4 MainPS(PixelInput input) : SV_Target
{
    return uint4(Constants.ObjectId, 0, 0, 0);
}
