#define SKYVIEWLUT_PASS
#define SOURCE_DISK_ENABLED
#define MULTISCATTERING_APPROX_SAMPLING_ENABLED

#include "skydefinitions.hlsli"

cbuffer Constants : register(b0)
{
    AtmosphereParameters Atmosphere;
    float4x4 SkyViewLutReferential;
    float4x4 InvViewProjection;
    float4 SkyViewLutSizeAndInvSize;
    float4 SkyPlanetTranslatedWorldCenterAndViewHeight;
    float3 AtmosphereLightDirection0;
    uint pad0;
    float3 AtmosphereLightIlluminanceOuterSpace0;
    uint pad1;
    float3 AtmosphereLightDirection1;
    uint pad2;
    float3 AtmosphereLightIlluminanceOuterSpace1;
    uint TransmittanceLutTextureDescriptorIndex;
    uint MultiScatteringLutTextureDescriptorIndex;
    uint SkyViewLutTextureDescriptorIndex;
}

Texture2D<float4> GetTransmittanceLUT()
{
    Texture2D<float4> tex = ResourceDescriptorHeap[TransmittanceLutTextureDescriptorIndex];
    return tex;
}

Texture2D<float4> GetMultiScatteringLUT()
{
    Texture2D<float4> tex = ResourceDescriptorHeap[MultiScatteringLutTextureDescriptorIndex];
    return tex;
}

SamplerState g_SamplerPointClamp : register(s0);
SamplerState g_SamplerBilinearClamp : register(s1);

#include "skyutils.hlsli"

// SkyViewLut is a new texture used for fast sky rendering.
// It is low resolution of the sky rendering around the camera,
// basically a lat/long parameterisation with more texel close to the horizon for more accuracy during sun set.

void UvToSkyViewLutParams(out float3 ViewDir, in float ViewHeight, in float2 UV)
{
	// Constrain uvs to valid sub texel range (avoid zenith derivative issue making LUT usage visible)
    UV = FromSubUvsToUnit(UV, SkyViewLutSizeAndInvSize);

    float Vhorizon = sqrt(ViewHeight * ViewHeight - Atmosphere.BottomRadiusKm * Atmosphere.BottomRadiusKm);
    float CosBeta = Vhorizon / ViewHeight; // cos of zenith angle from horizon to zenith
    float Beta = acosFast4(CosBeta);
    float ZenithHorizonAngle = PI - Beta;

    float ViewZenithAngle;
    if (UV.y < 0.5f)
    {
        float Coord = 2.0f * UV.y;
        Coord = 1.0f - Coord;
        Coord *= Coord;
        Coord = 1.0f - Coord;
        ViewZenithAngle = ZenithHorizonAngle * Coord;
    }
    else
    {
        float Coord = UV.y * 2.0f - 1.0f;
        Coord *= Coord;
        ViewZenithAngle = ZenithHorizonAngle + Beta * Coord;
    }
    float CosViewZenithAngle = cos(ViewZenithAngle);
    float SinViewZenithAngle = sqrt(1.0 - CosViewZenithAngle * CosViewZenithAngle) * (ViewZenithAngle > 0.0f ? 1.0f : -1.0f); // Equivalent to sin(ViewZenithAngle)

    float LongitudeViewCosAngle = UV.x * 2.0f * PI;

	// Make sure those values are in range as it could disrupt other math done later such as sqrt(1.0-c*c)
    float CosLongitudeViewCosAngle = cos(LongitudeViewCosAngle);
    float SinLongitudeViewCosAngle = sqrt(1.0 - CosLongitudeViewCosAngle * CosLongitudeViewCosAngle) * (LongitudeViewCosAngle <= PI ? 1.0f : -1.0f); // Equivalent to sin(LongitudeViewCosAngle)
    ViewDir = float3(
		SinViewZenithAngle * CosLongitudeViewCosAngle,
		SinViewZenithAngle * SinLongitudeViewCosAngle,
		CosViewZenithAngle
		);
}


[numthreads(8, 8, 1)]
void MainCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    RWTexture2D<float4> SkyView = ResourceDescriptorHeap[SkyViewLutTextureDescriptorIndex];
    
    float2 PixPos = float2(dispatchThreadID.xy) + 0.5f;
    float2 UV = PixPos * SkyViewLutSizeAndInvSize.zw;
    
    //float3 WorldPos = GetTranslatedCameraPlanetPos();
    
	// For the sky view lut to work, and not be distorted, we need to transform the view and light directions 
	// into a referential with UP being perpendicular to the world sphere. And with origin at the planet center.

	// This is the local referencial
    float3x3 LocalReferencial = GetUEReferential(SkyViewLutReferential);

	// This is the LUT camera height and position in the local referential
    float ViewHeight = SkyPlanetTranslatedWorldCenterAndViewHeight.w;
    float3 WorldPos = float3(0.0, 0.0, ViewHeight);

	// Get the view direction in this local referential
    float3 WorldDir;
    UvToSkyViewLutParams(WorldDir, ViewHeight, UV);
	// And also both light source direction
    float3 LightDir0 = -AtmosphereLightDirection0.zxy; //Negate as should be vector TO the light. Swizzling to account for unreal coord system
    LightDir0 = mul(LocalReferencial, LightDir0);
    float3 LightDir1 = -AtmosphereLightDirection1.zxy;
    LightDir1 = mul(LocalReferencial, LightDir1);


	// Move to top atmospehre
    if (!MoveToTopAtmosphere(WorldPos, WorldDir, Atmosphere.TopRadiusKm))
    {
		// Ray is not intersecting the atmosphere
        SkyView[int2(PixPos)] = 0.0f;
        return;
    }


    SamplingSetup Sampling = (SamplingSetup) 0;
	{
        Sampling.VariableSampleCount = true;
        Sampling.MinSampleCount = 4;
        Sampling.MaxSampleCount = 32;
        Sampling.DistanceToSampleCountMaxInv = 150; //km
    }
    const bool Ground = false;
    const float DeviceZ = 0; // Inverted depth
    const bool MieRayPhase = true;
    const float AerialPespectiveViewDistanceScale = 1.0f;
    
    SingleScatteringResult ss = IntegrateSingleScatteredLuminance(
		float4(PixPos, 0.0f, 1.0f), WorldPos, WorldDir,
		Ground, Sampling, DeviceZ, MieRayPhase,
		LightDir0, LightDir1,
		AtmosphereLightIlluminanceOuterSpace0, AtmosphereLightIlluminanceOuterSpace1,
		AerialPespectiveViewDistanceScale);

    const float Transmittance = dot(ss.Transmittance, float3(1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f));
    SkyView[int2(PixPos)] = float4(ss.L, Transmittance);
}