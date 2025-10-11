
struct ConstantsStruct
{
    float4x4 ViewProjection;
    float4x4 View;
    float4x4 ObjectTransform;
    
    uint VertexBufferDescriptorIndex;
    uint VertexPositionByteOffset;
    uint VertexNormalByteOffset;
    uint VertexStride;
    
    float2 OutlineSizeNdc;
    float ColorIntensity;
    float _pad;
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
    float3 localNormal = asfloat(vertexBuffer.Load3(vertexOffset + Constants.VertexNormalByteOffset));
    
    PixelInput output;
    float3 worldPosition = mul(Constants.ObjectTransform, float4(localPosition, 1.0f)).xyz;
    float3 worldNormal = normalize(mul(Constants.ObjectTransform, float4(localNormal, 0.0f)).xyz);
    float3 viewNormal = normalize(mul(Constants.View, float4(worldNormal, 0.0f)).xyz);
    output.clipPosition = mul(Constants.ViewProjection, float4(worldPosition, 1.0f));
    
    float2 ndc = output.clipPosition.xy / output.clipPosition.w;
    
    float2 screenDir = normalize(viewNormal.xy);
    float2 offset = screenDir * Constants.OutlineSizeNdc;
    
    ndc += offset;
    output.clipPosition.xy = ndc * output.clipPosition.w;
    
    return output;
}

float4 MainPS(PixelInput input) : SV_TARGET
{
    return float4(1.0f, 1.0f, 0.0f, 1.0f) * Constants.ColorIntensity;
}
