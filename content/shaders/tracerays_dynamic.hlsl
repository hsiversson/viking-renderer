#include "common.hlsli"
#include "raytracingcommon.hlsli"
#include "sceneconstants.hlsli"
#include "skydefinitions.hlsli"

cbuffer Constants : register(b0)
{
    AtmosphereParameters Atmosphere;
    uint TargetTextureDescriptorIndex;
    uint DiffuseAlbedoTextureDescriptor;
    uint SpecularAlbedoTextureDescriptor;
    uint NormalRoughnessTextureDescriptor;
    uint SpecularHitDistanceTextureDescriptor;
    uint TransmittanceTextureDescriptorIndex;
    uint2 pad;
};

SamplerState g_SamplerPointClamp : register(s0);
SamplerState g_SamplerBilinearClamp : register(s1);

[shader("raygeneration")]
void TraceRays()
{
    const uint2 pixel = DispatchRaysIndex().xy;
    
    RWTexture2D<float4> target = ResourceDescriptorHeap[TargetTextureDescriptorIndex];
    uint width, height;
    target.GetDimensions(width, height);
    
    const float2 texelSize = 1.0f / float2(width, height);
    const float2 uv = (pixel + 0.5f) * texelSize;
    const float2 ndc = float2(uv.x * 2.0f - 1.0f, (1.0f - uv.y) * 2.0f - 1.0f);
    
    //Unproject pixel to get ray origin and direction
    float4 clipPos = float4(ndc, 0.0f, 1.0f);
    float4 worldPos = mul(SceneConstants.InvViewProjection, clipPos);
    worldPos /= worldPos.w;
    float3 ro = SceneConstants.CameraPosition;
    float3 rd = normalize(worldPos.xyz - SceneConstants.CameraPosition);
    
    RayDesc ray;
    ray.Origin = ro;
    ray.Direction = rd;
    ray.TMin = 0.001f;
    ray.TMax = 1000000.0f;
    
    RaytracingAccelerationStructure RaytracingScene = ResourceDescriptorHeap[SceneConstants.RaytracingSceneDescriptorIndex];
    
    uint flags = RAY_FLAG_NONE;
    flags |= RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES;
    
    RaytracingPayload payload = (RaytracingPayload)0;
    payload.rngState = GenerateRandomSeed(pixel.x, pixel.y, SceneConstants.FrameIndex);
    
    TraceRay(RaytracingScene, flags, 0xff, 0, 0, 0, ray, payload);

    target[pixel] = float4(payload.irradiance, 1.0f);
    
    // write normals for denoising
    RWTexture2D<float4> normalRoughnessTarget = ResourceDescriptorHeap[NormalRoughnessTextureDescriptor];
    normalRoughnessTarget[pixel] = float4(payload.worldNormal, payload.roughness);
    
    RWTexture2D<float4> diffuseAlbedoTarget = ResourceDescriptorHeap[DiffuseAlbedoTextureDescriptor];
    diffuseAlbedoTarget[pixel] = float4(payload.diffuseAlbedo, 0.0f);
    
    RWTexture2D<float4> specularAlbedoTarget = ResourceDescriptorHeap[SpecularAlbedoTextureDescriptor];
    specularAlbedoTarget[pixel] = float4(payload.specularAlbedo, 0.0f);
    
    RWTexture2D<float> specularHitDistanceTarget = ResourceDescriptorHeap[SpecularHitDistanceTextureDescriptor];
    specularHitDistanceTarget[pixel] = payload.specularHitDistance;
}

Texture2D<float4> GetTransmittanceLUT()
{
    Texture2D<float4> tex = ResourceDescriptorHeap[TransmittanceTextureDescriptorIndex];
    return tex;
}

#define RENDERSKY_ENABLED
#define SOURCE_DISK_ENABLED
#include "skyutils.hlsli"

float3 GetLightDiskLuminance(float3 PlanetCenterToCamera, float3 WorldDir, uint LightIndex)
{
    float t = RaySphereIntersectNearest(PlanetCenterToCamera, WorldDir, float3(0.0f, 0.0f, 0.0f), Atmosphere.BottomRadiusKm);
    if (t < 0.0f)												// No intersection with the planet
		//&& View.RenderingReflectionCaptureMask==0.0f)	// Do not render light disk when in reflection capture in order to avoid double specular. The sun contribution is already computed analyticaly.
    {
        // Note the correction in light direction (negation as Unreal treats light dir as vector to the light, and swizzling to counter Unreal differences in coord system)
		// GetLightDiskLuminance contains a tiny soft edge effect
        float3 LightDiskLuminance = GetLightDiskLuminance(
			PlanetCenterToCamera, WorldDir, Atmosphere.BottomRadiusKm, Atmosphere.TopRadiusKm,
			GetTransmittanceLUT(), g_SamplerBilinearClamp,
			-SceneConstants.DirectionalLights[LightIndex].Direction.zxy, cos(SceneConstants.DirectionalLights[LightIndex].Radius), SceneConstants.DirectionalLights[LightIndex].Emission);

		// Clamp to avoid crazy high values (and exposed 64000.0f luminance is already crazy high, solar system sun is 1.6x10^9). Also this removes +inf float and helps TAA.
        const float3 MaxLightLuminance = 64000.0f;
        float3 ExposedLightLuminance = LightDiskLuminance * OutputPreExposure;
        ExposedLightLuminance = min(ExposedLightLuminance, MaxLightLuminance);

        return ExposedLightLuminance;
    }
    return 0.0f;
}

float4 PrepareOutput(float3 PreExposedLuminance, float3 Transmittance = float3(1.0f, 1.0f, 1.0f))
{
    const static float Max10BitsFloat = 64512.0f;
	// Sky materials can result in high luminance values, e.g. the sun disk. 
	// This is so we make sure to at least stay within the boundaries of fp10 and not cause NaN on some platforms.
	// We also half that range to also make sure we have room for other additive elements such as bloom, clouds or particle visual effects.
    const float3 SafePreExposedLuminance = min(PreExposedLuminance, Max10BitsFloat.xxx * 0.5f);
	
    const float GreyScaleTransmittance = dot(Transmittance, float3(1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f));
    float4 LuminanceAlpha = float4(SafePreExposedLuminance, GreyScaleTransmittance);
	
    //No idea what this does. some unreal magic
//    [flatten]
//    if (bPropagateAlphaNonReflection > 0)
//    {
//#if SUPPORT_PRIMITIVE_ALPHA_HOLDOUT
//		LuminanceAlpha.rgb	= IsSkyAtmosphereHoldout(View.EnvironmentComponentsFlags) ? 0.0f : LuminanceAlpha.rgb;
//#endif
//        LuminanceAlpha.a = 1 - GreyScaleTransmittance;
//    }
	
    return LuminanceAlpha;

}

[shader("miss")]
void Miss(inout RaytracingPayload payload)
{
    //Sky rendering
  
    float4 OutLuminance = 0;
    payload.specularHitDistance = RayTCurrent();
    
    float2 PixPos = DispatchRaysIndex().xy;
    RWTexture2D<float4> target = ResourceDescriptorHeap[TargetTextureDescriptorIndex];
    uint width, height;
    target.GetDimensions(width, height);
    const float2 texelSize = 1.0f / float2(width, height);
    float2 UvBuffer = PixPos * texelSize; // Uv for depth buffer read (size can be larger than viewport)
    
    
	// World position are relative to the planet center (itself expressed within translated world space)
    float3 WorldDir = WorldRayDirection(); //GetScreenWorldDir(PixPos);
    //Were using for sky calculations the Unreal coordinate system. Which is a left handed system but with Z up , X forward and Y right
    //Our reference system uses Y up, Z forward and X right instead
    WorldDir = WorldDir.zxy;
    //float3 WorldPos = GetTranslatedCameraPlanetPos();
    // TODO: Lets assume planet center is below us and were always 5 meters above surface. In the future we need to set a reference to count camera height
    float3 WorldPos = float3(0, 0, Atmosphere.BottomRadiusKm + 0.005f);
    //if (IsOrthoProjection())
    //{
    //    WorldPos += GetTranslatedWorldCameraPosFromView(SVPos.xy, true);
    //}

    float3 PreExposedL = 0;
    float3 LuminanceScale = 1.0f;

#if SAMPLE_ATMOSPHERE_ON_CLOUDS
	// We could read cloud color and skip if transmittance<0.999. Could do that if it would be a compute shader.

	const float4 CloudLuminanceTransmittance = InputCloudLuminanceTransmittanceTexture.Load(int3(PixPos, 0));
	const float CloudCoverage = 1.0f - CloudLuminanceTransmittance.a;
	if (CloudLuminanceTransmittance.a > 0.999)
	{
		OutLuminance = float4(0.0f, 0.0f, 0.0f, 1.0f);
		return;
	}

	const float CloudDepthKm = VolumetricCloudDepthTexture.Load(int3(PixPos, 0)).r;
	float DeviceZ = CloudDepthKm; // Warning: for simplicity, we use DeviceZ as world distance in kilometer when SAMPLE_ATMOSPHERE_ON_CLOUDS. See special case in IntegrateSingleScatteredLuminance.

#else // SAMPLE_ATMOSPHERE_ON_CLOUDS

//#if  MSAA_SAMPLE_COUNT > 1
//	float DeviceZ = DepthReadDisabled ? FarDepthValue : MSAADepthTexture.Load(int2(PixPos), SampleIndex).x;
//#else
//    float DeviceZ = DepthReadDisabled ? FarDepthValue : LookupDeviceZ(UvBuffer);
//#endif
    
    float DeviceZ = FarDepthValue;

    if (DeviceZ == FarDepthValue)
    {
		// Get the light disk luminance to draw 
        LuminanceScale = float3(1, 1, 1); //Constants.Atmosphere.SkyLuminanceFactor;
#ifdef SOURCE_DISK_ENABLED
		PreExposedL += GetLightDiskLuminance(WorldPos, WorldDir, 0);
#ifdef SECOND_ATMOSPHERE_LIGHT_ENABLED
		PreExposedL += GetLightDiskLuminance(WorldPos, WorldDir, 1);
#endif
#endif

#ifndef RENDERSKY_ENABLED
		// We should not render the sky and the current pixels are at far depth, so simply early exit.
		// We enable depth bound when supported to not have to even process those pixels.
		OutLuminance = PrepareOutput(float3(0.0f, 0.0f, 0.0f), float3(1.0f, 1.0f, 1.0f));

		//Now the sky pass can ignore the pixel with depth == far but it will need to alpha clip because not all RHI backend support depthbound tests.
		// And the depthtest is already setup to avoid writing all the pixel closer than to the camera than the start distance (very good optimisation).
		// Since this shader does not write to depth or stencil it should still benefit from EArlyZ even with the clip (See AMD depth-in-depth documentation)
		clip(-1.0f);
		return;
#endif
    }
    //else if (SkyAtmosphere.FogShowFlagFactor <= 0.0f)
    //{
    //    OutLuminance = PrepareOutput(float3(0.0f, 0.0f, 0.0f), float3(1.0f, 1.0f, 1.0f));
    //    clip(-1.0f);
    //    return;
    //}
#endif // SAMPLE_ATMOSPHERE_ON_CLOUDS

    float ViewHeight = length(WorldPos);
#if defined(FASTSKY_ENABLED) && defined(RENDERSKY_ENABLED)
	if (ViewHeight < (Atmosphere.TopRadiusKm * PLANET_RADIUS_RATIO_SAFE_EDGE) && DeviceZ == FarDepthValue
		&& (View.RenderingReflectionCaptureMask > 0.0f || IsSkyAtmosphereRenderedInMain(View.EnvironmentComponentsFlags)))
	{
		float2 UV;

		// The referencial used to build the Sky View lut
		float3x3 LocalReferencial = GetSkyViewLutReferential(View.SkyViewLutReferential);

		// Input vectors expressed in this referencial: Up is always Z. Also note that ViewHeight is unchanged in this referencial.
		float3 WorldPosLocal = float3(0.0, 0.0, ViewHeight);
		float3 UpVectorLocal = float3(0.0, 0.0, 1.0);
		float3 WorldDirLocal = mul(LocalReferencial, WorldDir);
		float ViewZenithCosAngle = dot(WorldDirLocal, UpVectorLocal);

		// Now evaluate inputs in the referential
		bool IntersectGround = RaySphereIntersectNearest(WorldPosLocal, WorldDirLocal, float3(0, 0, 0), Atmosphere.BottomRadiusKm) >= 0.0f;

		SkyViewLutParamsToUv(IntersectGround, ViewZenithCosAngle, WorldDirLocal, ViewHeight, Atmosphere.BottomRadiusKm, SkyAtmosphere.SkyViewLutSizeAndInvSize, UV);
		float4 SkyLuminanceTransmittance = SkyViewLutTexture.SampleLevel(SkyViewLutTextureSampler, UV, 0);
		float3 SkyLuminance = SkyLuminanceTransmittance.rgb;

		float3 SkyGreyTransmittance = 1.0f;
		FLATTEN
		if(bPropagateAlphaNonReflection > 0)
		{
			SkyGreyTransmittance = SkyLuminanceTransmittance.aaa;
		}

		PreExposedL += SkyLuminance * LuminanceScale * (ViewOneOverPreExposure * OutputPreExposure);
	
		OutLuminance = PrepareOutput(PreExposedL, SkyGreyTransmittance);
		UpdateVisibleSkyAlpha(DeviceZ, OutLuminance);
		return;
	}
#endif

#if FASTAERIALPERSPECTIVE_ENABLED

#if COLORED_TRANSMITTANCE_ENABLED
#error The FASTAERIALPERSPECTIVE_ENABLED path does not support COLORED_TRANSMITTANCE_ENABLED.
#else

	float3 DepthBufferTranslatedWorldPos = GetScreenTranslatedWorldPos(SVPos, DeviceZ).xyz;
	float4 NDCPosition = mul(float4(DepthBufferTranslatedWorldPos.xyz, 1), View.TranslatedWorldToClip);

	const float NearFadeOutRangeInvDepthKm = 1.0 / 0.00001f; // 1 centimeter fade region
	float4 AP = GetAerialPerspectiveLuminanceTransmittance(
		ResolvedView.RealTimeReflectionCapture, ResolvedView.SkyAtmosphereCameraAerialPerspectiveVolumeSizeAndInvSize,
		NDCPosition, (DepthBufferTranslatedWorldPos - GetCameraTranslatedWorldPos()) * CM_TO_SKY_UNIT,
		CameraAerialPerspectiveVolumeTexture, CameraAerialPerspectiveVolumeTextureSampler,
		SkyAtmosphere.CameraAerialPerspectiveVolumeDepthResolutionInv,
		SkyAtmosphere.CameraAerialPerspectiveVolumeDepthResolution,
		AerialPerspectiveStartDepthKm,
		SkyAtmosphere.CameraAerialPerspectiveVolumeDepthSliceLengthKm,
		SkyAtmosphere.CameraAerialPerspectiveVolumeDepthSliceLengthKmInv,
		ViewOneOverPreExposure * OutputPreExposure,
		NearFadeOutRangeInvDepthKm);

	PreExposedL += AP.rgb * LuminanceScale;
	float Transmittance = AP.a;

	OutLuminance = PrepareOutput(PreExposedL, float3(Transmittance, Transmittance, Transmittance));
	UpdateVisibleSkyAlpha(DeviceZ, OutLuminance);
	return;
#endif

#else // FASTAERIALPERSPECTIVE_ENABLED

	// Move to top atmosphere as the starting point for ray marching.
	// This is critical to be after the above to not disrupt above atmosphere tests and voxel selection.
    if (!MoveToTopAtmosphere(WorldPos, WorldDir, Atmosphere.TopRadiusKm))
    {
		// Ray is not intersecting the atmosphere
        OutLuminance = PrepareOutput(PreExposedL);
        return;
    }

	// Apply the start depth offset after moving to the top of atmosphere for consistency (and to avoid wrong out-of-atmosphere test resulting in black pixels).
    const float AerialPerspectiveStartDepthKm = 0.1; //TODO: Take from atmosphere constants
    WorldPos += WorldDir * AerialPerspectiveStartDepthKm;
    
    //TODO: Get these constants from the atmosphere uniform buffer 
    const float BaseSampleCount = 32.0f;
    const float SampleCountMin = 2.0f;
    const float SampleCountMax = 32.0f;
    //Distance in km after which SampleCountMax samples will be used to ray march the atmosphere
    const float DistanceToSampleCountMaxInv = 1.0f / 150.0f;

    SamplingSetup Sampling = (SamplingSetup) 0;
	{
        Sampling.VariableSampleCount = true;
        Sampling.MinSampleCount = SampleCountMin;
        Sampling.MaxSampleCount = SampleCountMax;
        Sampling.DistanceToSampleCountMaxInv = DistanceToSampleCountMaxInv;
    }
    const bool Ground = false;
    const bool MieRayPhase = true;
    //const float AerialPespectiveViewDistanceScale = DeviceZ == FarDepthValue ? 1.0f : SkyAtmosphere.AerialPespectiveViewDistanceScale;
    const float AerialPespectiveViewDistanceScale = 1.0f;
    SingleScatteringResult ss = IntegrateSingleScatteredLuminance(
		float4(PixPos, 0.0f, 1.0f), WorldPos, WorldDir,
		Ground, Sampling, DeviceZ, MieRayPhase,
		-SceneConstants.DirectionalLights[0].Direction.zxy, -SceneConstants.DirectionalLights[1].Direction.zxy,
		SceneConstants.DirectionalLights[0].Emission, SceneConstants.DirectionalLights[1].Emission,
		AerialPespectiveViewDistanceScale);

    PreExposedL += ss.L * LuminanceScale;
	
    //if (View.RenderingReflectionCaptureMask == 0.0f && !IsSkyAtmosphereRenderedInMain(View.EnvironmentComponentsFlags))
    //{
    //    PreExposedL = 0.0f;
    //}

#if SAMPLE_ATMOSPHERE_ON_CLOUDS
	// We use gray scale transmittance to match the rendering when applying the AerialPerspective texture
	const float GreyScaleAtmosphereTransmittance = dot(ss.Transmittance, float3(1.0 / 3.0f, 1.0 / 3.0f, 1.0 / 3.0f));
	// Reduce cloud luminance according to the atmosphere transmittance and add the atmosphere in scattred luminance according to the cloud coverage.
	PreExposedL = CloudLuminanceTransmittance.rgb * GreyScaleAtmosphereTransmittance + CloudCoverage * PreExposedL;
	// Coverage of the cloud layer itself does not change.
	ss.Transmittance = CloudLuminanceTransmittance.a;
#endif

#if COLORED_TRANSMITTANCE_ENABLED
#error Requires support for dual source blending.
	output.Luminance = float4(PreExposedL, 1.0f);
	output.Transmittance = float4(ss.Transmittance, 1.0f);
#else
    payload.irradiance = PrepareOutput(PreExposedL, ss.Transmittance).xyz;
    //UpdateVisibleSkyAlpha(DeviceZ, OutLuminance);
    return;
#endif

#endif // FASTAERIALPERSPECTIVE_ENABLED
    
}