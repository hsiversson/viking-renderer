#define AERIALPERSPECTIVE_PASS

#include "skydefinitions.hlsli"

#define MULTISCATTERING_APPROX_SAMPLING_ENABLED

cbuffer Constants : register(b0)
{
    AtmosphereParameters Atmosphere;
    
    float4x4 InvViewProjection;
    
    float4 AerialPerspectiveLutSizeAndInvSize;
    
    float4 SkyPlanetTranslatedWorldCenterAndViewHeight;
    
    float4 CameraPosition;
    
    float4 ViewSizeAndInvSize;
    
    float3 AtmosphereLightDirection0;
    uint pad0;
    
    float3 AtmosphereLightIlluminanceOuterSpace0;
    uint pad1;
    
    float3 AtmosphereLightDirection1;
    uint pad2;
    
    float3 AtmosphereLightIlluminanceOuterSpace1;
    uint pad3;
    
    float FogShowFlagFactor;
    float2 AerialPerspectiveLutDepthAndInvDepth;
    float AerialPerspectiveStartDepthKm;
    
    float CameraAerialPerspectiveVolumeDepthSliceLengthKm;
    uint TransmittanceLUTTextureDescriptorIndex;
    uint MultiscatterLUTTextureDescriptorIndex;
    uint AerialPerspectiveTextureDescriptorIndex;
    
#if SEPARATE_MIE_RAYLEIGH_SCATTERING // Relevant only in conjuntion with volumetric cloud rendering
    uint CameraAerialPerspectiveVolumeMieOnlyUAV;
    uint CameraAerialPerspectiveVolumeRayOnlyUAV;
#endif
}

Texture2D<float4> GetTransmittanceLUT()
{
    Texture2D<float4> tex = ResourceDescriptorHeap[TransmittanceLUTTextureDescriptorIndex];
    return tex;
}

Texture2D<float4> GetMultiScatteringLUT()
{
    Texture2D<float4> tex = ResourceDescriptorHeap[MultiscatterLUTTextureDescriptorIndex];
    return tex;
}

SamplerState g_SamplerPointClamp : register(s0);
SamplerState g_SamplerBilinearClamp : register(s1);

#include "skyutils.hlsli"

#define AERIAL_PERSPECTIVE_SAMPLE_COUNT_PER_SLICE 2 //TODO: In UNreal is configurable. Do we want to do that?

//The aerial perspective texture is a low res 3D volume fitted to the camera frustum that

[numthreads(8, 8, 8)]
void MainCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    RWTexture3D<float4> AerialPerspective = ResourceDescriptorHeap[AerialPerspectiveTextureDescriptorIndex];
    if (FogShowFlagFactor <= 0.0f)
    {
        AerialPerspective[dispatchThreadID] = float4(0.0f, 0.0f, 0.0f, 1.0f);
#if SEPARATE_MIE_RAYLEIGH_SCATTERING
		AerialPerspectiveMieOnly[dispatchThreadID] = float4(0.0f, 0.0f, 0.0f, 1.0f);
		AerialPerspectiveRayOnly[dispatchThreadID] = float4(0.0f, 0.0f, 0.0f, 1.0f);
#endif
        return;
    }
    
    float2 PixPos = float2(dispatchThreadID.xy) + 0.5f;
    float2 UV = PixPos * AerialPerspectiveLutSizeAndInvSize.zw;

    //We assume we start the view at 0,0
    float4 SVPos = float4(/*View.ViewRectMin.xy +*/ UV * ViewSizeAndInvSize.xy, 0.0f, 1.0f); // SV_POS as if resolution was the one from the scene view.
    
    const float2 ndc = float2(UV.x * 2.0f - 1.0f, (1.0f - UV.y) * 2.0f - 1.0f);
    
    //Unproject pixel to get ray origin and direction
    float4 clipPos = float4(ndc, 0.0f, 1.0f);
    float4 worldPos = mul(InvViewProjection, clipPos);
    worldPos /= worldPos.w;
    
    float3 WorldDir = normalize(worldPos - CameraPosition); //GetScreenWorldDir(SVPos);
    WorldDir = WorldDir.zxy; // Convert to UE coord system
    
    //Camera position relative to the virtual planet center. Notice the swizzling to convert to Unreal coordinate system
    float3 CamPos = -SkyPlanetTranslatedWorldCenterAndViewHeight.zxy;
    //if (IsOrthoProjection())
    //{
    //    CamPos += GetTranslatedWorldCameraPosFromView(SVPos.xy, true);
    //}

    //Huh?
  //  if (RealTimeReflection360Mode)
  //  {
  //      float2 UnitUV = FromSubUvsToUnit(UV, AerialPerspectiveLutSizeAndInvSize);

		//// Simple lat-long mapping  with with UV.y=sin(ElevationAngle)
  //      float SinPhi = 2.0f * UnitUV.y - 1.0f;
  //      float CosPhi = sqrt(1.0f - SinPhi * SinPhi);
  //      float Theta = 2.0f * PI * UnitUV.x;
  //      float CosTheta = cos(Theta);
  //      float SinTheta = sqrt(1.0f - CosTheta * CosTheta) * (Theta > PI ? -1.0f : 1.0f);
  //      WorldDir = float3(CosTheta * CosPhi, SinTheta * CosPhi, SinPhi);
  //      WorldDir = normalize(WorldDir);
  //  }

    float Slice = ((float(dispatchThreadID.z) + 0.5f) * AerialPerspectiveLutDepthAndInvDepth.y); // +0.5 to always have a distance to integrate over
    Slice *= Slice; // squared distribution
    Slice *= AerialPerspectiveLutDepthAndInvDepth.x;

    float3 RayStartWorldPos = CamPos + AerialPerspectiveStartDepthKm * WorldDir; // Offset according to start depth
    float ViewHeight;


	// Compute position from froxel information
    float tMax = Slice * CameraAerialPerspectiveVolumeDepthSliceLengthKm;
    float3 VoxelWorldPos = RayStartWorldPos + tMax * WorldDir;
    float VoxelHeight = length(VoxelWorldPos);

	// Check if the voxel is under the horizon.
    const float UnderGround = VoxelHeight < Atmosphere.BottomRadiusKm;

	// Check if the voxel is beind the planet (to next check for below the horizon case)
    float3 CameraToVoxel = VoxelWorldPos - CamPos;
    float CameraToVoxelLen = length(CameraToVoxel);
    float3 CameraToVoxelDir = CameraToVoxel / CameraToVoxelLen;
    float PlanetNearT = RaySphereIntersectNearest(CamPos, CameraToVoxelDir, float3(0, 0, 0), Atmosphere.BottomRadiusKm);
    bool BelowHorizon = PlanetNearT > 0.0f && CameraToVoxelLen > PlanetNearT;

    if (BelowHorizon || UnderGround)
    {
        CamPos += normalize(CamPos) * 0.02f; // TODO: investigate why we need this workaround. Without it, we get some bad color and flickering on the ground only (floating point issue with sphere intersection code?).

        float3 VoxelWorldPosNorm = normalize(VoxelWorldPos);
        float3 CamProjOnGround = normalize(CamPos) * Atmosphere.BottomRadiusKm;
        float3 VoxProjOnGround = VoxelWorldPosNorm * Atmosphere.BottomRadiusKm;
        float3 VoxelGroundToRayStart = CamPos - VoxProjOnGround;
        if (BelowHorizon && dot(normalize(VoxelGroundToRayStart), VoxelWorldPosNorm) < 0.0001f)
        {
			// We are behind the sphere and the sphere normal is pointing away from V: we are below the horizon.
            float3 MiddlePoint = 0.5f * (CamProjOnGround + VoxProjOnGround);
            float MiddlePointHeight = length(MiddlePoint);

			// Compute the new position to evaluate and store the value in the voxel.
			// the position is the oposite side of the horizon point from the view point,
			// The offset of 1.001f is needed to get matching colors and for the ray to not hit the earth again later due to floating point accuracy
            float3 MiddlePointOnGround = normalize(MiddlePoint) * Atmosphere.BottomRadiusKm; // *1.001f;
            VoxelWorldPos = CamPos + 2.0f * (MiddlePointOnGround - CamPos);

			//CameraAerialPerspectiveVolumeUAV[ThreadId] = float4(1, 0, 0, 0);
			//#if SEPARATE_MIE_RAYLEIGH_SCATTERING
			//	CameraAerialPerspectiveVolumeMieOnlyUAV[ThreadId] = float4(1, 0, 0, 0);
			//	CameraAerialPerspectiveVolumeRayOnlyUAV[ThreadId] = float4(1, 0, 0, 0);
			//#endif
			//return; // debug
        }
        else if (UnderGround)
        {
			//No obstruction from the planet, so use the point on the ground
            VoxelWorldPos = normalize(VoxelWorldPos) * (Atmosphere.BottomRadiusKm);
			//VoxelWorldPos = CamPos + CameraToVoxelDir * PlanetNearT;		// better match but gives visual artefact as visible voxels on a simple plane at altitude 0

			//CameraAerialPerspectiveVolumeUAV[ThreadId] = float4(0, 1, 0, 0);
			//#if SEPARATE_MIE_RAYLEIGH_SCATTERING
			//	CameraAerialPerspectiveVolumeMieOnlyUAV[ThreadId] = float4(0, 1, 0, 0);
			//	CameraAerialPerspectiveVolumeRayOnlyUAV[ThreadId] = float4(0, 1, 0, 0);
			//#endif
			//return; // debug
        }
		 
        WorldDir = normalize(VoxelWorldPos - CamPos);
        RayStartWorldPos = CamPos + AerialPerspectiveStartDepthKm * WorldDir; // Offset according to start depth
        tMax = length(VoxelWorldPos - RayStartWorldPos);
    }
    float tMaxMax = tMax;

	// Move ray marching start up to top atmosphere.
    ViewHeight = length(RayStartWorldPos);
    if (ViewHeight >= Atmosphere.TopRadiusKm)
    {
        float3 prevWorlPos = RayStartWorldPos;
        if (!MoveToTopAtmosphere(RayStartWorldPos, WorldDir, Atmosphere.TopRadiusKm))
        {
			// Ray is not intersecting the atmosphere
            AerialPerspective[dispatchThreadID] = float4(0.0f, 0.0f, 0.0f, 1.0f);
#if SEPARATE_MIE_RAYLEIGH_SCATTERING
			AerialPerspectiveMieOnly[dispatchThreadID] = float4(0.0f, 0.0f, 0.0f, 1.0f);
			AerialPerspectiveRayOnly[dispatchThreadID] = float4(0.0f, 0.0f, 0.0f, 1.0f);
#endif
            return;
        }
        float LengthToAtmosphere = length(prevWorlPos - RayStartWorldPos);
        if (tMaxMax < LengthToAtmosphere)
        {
			// tMaxMax for this voxel is not within the planet atmosphere
            AerialPerspective[dispatchThreadID] = float4(0.0f, 0.0f, 0.0f, 1.0f);
#if SEPARATE_MIE_RAYLEIGH_SCATTERING
			AerialPerspectiveMieOnly[dispatchThreadID] = float4(0.0f, 0.0f, 0.0f, 1.0f);
			AerialPerspectiveRayOnly[dispatchThreadID] = float4(0.0f, 0.0f, 0.0f, 1.0f);
#endif
            return;
        }
		// Now world position has been moved to the atmosphere boundary: we need to reduce tMaxMax accordingly. 
        tMaxMax = max(0.0, tMaxMax - LengthToAtmosphere);
    }


    SamplingSetup Sampling = (SamplingSetup) 0;
	{
        Sampling.VariableSampleCount = false;
        Sampling.SampleCountIni = max(1.0f, (float(dispatchThreadID.z) + 1.0f) * AERIAL_PERSPECTIVE_SAMPLE_COUNT_PER_SLICE);
    }
    const bool Ground = false;
    const float DeviceZ = FarDepthValue;
    const bool MieRayPhase = true;
    const float AerialPespectiveViewDistanceScale = 1.0f; //TODO: Get from sky constants
    float3 LightDir0 = -AtmosphereLightDirection0.zxy;
    float3 LightDir1 = -AtmosphereLightDirection1.zxy;
    SingleScatteringResult ss = IntegrateSingleScatteredLuminance(
		float4(PixPos, 0.0f, 1.0f), RayStartWorldPos, WorldDir,
		Ground, Sampling, DeviceZ, MieRayPhase,
		LightDir0, LightDir1,
		AtmosphereLightIlluminanceOuterSpace0, AtmosphereLightIlluminanceOuterSpace1,
		AerialPespectiveViewDistanceScale,
		tMaxMax);

#if SUPPORT_PRIMITIVE_ALPHA_HOLDOUT
	if (IsSkyAtmosphereHoldout(View.EnvironmentComponentsFlags) && !RealTimeReflection360Mode)
	{
		ss.L *= 0;
		ss.LMieOnly *= 0;
		ss.LRayOnly *= 0;
	}
#endif

    const float Transmittance = dot(ss.Transmittance, float3(1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f));
    AerialPerspective[dispatchThreadID] = float4(ss.L, Transmittance);
#if SEPARATE_MIE_RAYLEIGH_SCATTERING
		const float TransmittanceMieOnly = dot(ss.TransmittanceMieOnly, float3(1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f));
		const float TransmittanceRayOnly = dot(ss.TransmittanceRayOnly, float3(1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f));
		AerialPerspectiveMieOnly[dispatchThreadID]	= float4(ss.LMieOnly, TransmittanceMieOnly);
		AerialPerspectiveRayOnly[dispatchThreadID]	= float4(ss.LRayOnly, TransmittanceRayOnly);
#endif
}