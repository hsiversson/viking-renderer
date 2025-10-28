#define WHITE_TRANSMITTANCE
#define TRANSMITTANCE_PASS

#include "skydefinitions.hlsli"

cbuffer Constants : register(b0)
{
    AtmosphereParameters Atmosphere;
    float4 TransmittanceLutSizeAndInvSize;
    uint TransmittanceTextureDescriptorIndex;
}

#include "skyutils.hlsli"

#define TRANSMITTANCE_SAMPLE_COUNT 10


[numthreads(8, 8, 1)]
void MainCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    RWTexture2D<float4> Transmittance = ResourceDescriptorHeap[TransmittanceTextureDescriptorIndex];
    float2 PixPos = float2(dispatchThreadID.xy) + 0.5f;

	// Compute camera position from LUT coords
    float2 UV = (PixPos) * TransmittanceLutSizeAndInvSize.zw;
    float ViewHeight;
    float ViewZenithCosAngle;
    UvToLutTransmittanceParams(ViewHeight, ViewZenithCosAngle, UV);

	//  A few extra needed constants
    float3 WorldPos = float3(0.0f, 0.0f, ViewHeight);
    float3 WorldDir = float3(0.0f, sqrt(1.0f - ViewZenithCosAngle * ViewZenithCosAngle), ViewZenithCosAngle);

    SamplingSetup Sampling = (SamplingSetup) 0;
	{
        Sampling.VariableSampleCount = false;
        Sampling.SampleCountIni = TRANSMITTANCE_SAMPLE_COUNT;
    }
    const bool Ground = false;
    const float DeviceZ = 0.0f; //0 is our far depth value as we use inverted depth
    const bool MieRayPhase = false;
    const float3 NullLightDirection = float3(0.0f, 0.0f, 1.0f);
    const float3 NullLightIlluminance = float3(0.0f, 0.0f, 0.0f);
    const float AerialPespectiveViewDistanceScale = 1.0f; //TODO: Get from sky constants
    SingleScatteringResult ss = IntegrateSingleScatteredLuminance(
		float4(PixPos, 0.0f, 1.0f), WorldPos, WorldDir,
		Ground, Sampling, DeviceZ, MieRayPhase,
		NullLightDirection, NullLightDirection, NullLightIlluminance, NullLightIlluminance,
		AerialPespectiveViewDistanceScale);

    float3 transmittance = exp(-ss.OpticalDepth);
    
    Transmittance[int2(PixPos)] = float4(transmittance, 0.0f);
}