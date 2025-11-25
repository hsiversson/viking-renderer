#include "sky.h"

#include "core/color.h"
#include "core/common.h"
#include "render/profiler.h"
#include "view.h"
#include "viewrenderdata.h"

using namespace vkr;

namespace
{
	constexpr uint32_t TRANSMITTANCE_TEXTURE_WIDTH = 256;
	constexpr uint32_t TRANSMITTANCE_TEXTURE_HEIGHT = 64;

	constexpr uint32_t MULTISCATTERING_TEXTURE_WIDTH = 32;
	constexpr uint32_t MULTISCATTERING_TEXTURE_HEIGHT = 32;

	constexpr uint32_t SKYVIEW_TEXTURE_WIDTH = 192;
	constexpr uint32_t SKYVIEW_TEXTURE_HEIGHT = 104;

	constexpr uint32_t AERIAL_PERSPECTIVE_TEXTURE_WIDTHHEIGHT = 32;
	constexpr uint32_t AERIAL_PERSPECTIVE_TEXTURE_DEPTH = 16;

	static constexpr float SkyUnitToVkrUnits = 1000.0f; //Kilometers to meters
	static constexpr float VkrUnitsToSkyUnits = 0.001f; //Meters to kilometers
}

namespace vkr::Graphics
{
	AtmosphereParams::AtmosphereParams()
	{
		// All distance here are in kilometer and scattering/absorptions coefficient in 1/kilometers.
		const float EarthBottomRadius = 6360.0f;
		const float EarthTopRadius = 6420.0f;
		const float EarthRayleighScaleHeight = 8.0f;
		const float EarthMieScaleHeight = 1.2f;

		BottomRadius = EarthBottomRadius;
		AtmosphereHeight = EarthTopRadius - EarthBottomRadius;
		GroundAlbedo = Vector3f(170.0f/255.0f, 170.0f / 255.0f, 170.0f / 255.0f); // This is an sRGB value. Corresponds to  0.4f in linear space

		// Float to a u8 rgb + float length can lose some precision but it is better UI wise.
		const Vector3f RayleightScatteringRaw = Vector3f(0.005802f, 0.013558f, 0.033100f); //This is directly linear color
		RayleighScattering = RayleightScatteringRaw * (1.0f / RayleightScatteringRaw.z);
		RayleighScatteringScale = RayleightScatteringRaw.z;
		RayleighExponentialDistribution = EarthRayleighScaleHeight;

		MieScattering = Vector3f(1.0f, 1.0f, 1.0f);
		MieScatteringScale = 0.003996f;
		MieAbsorption = Vector3f(1.0f, 1.0f, 1.0f);
		MieAbsorptionScale = 0.000444f;
		MieAnisotropy = 0.8f; //Referred in Bruneton as PhaseG
		MieExponentialDistribution = EarthMieScaleHeight;

		// Absorption tent distribution representing ozone distribution in Earth atmosphere.
		const Vector3f OtherAbsorptionRaw = Vector3f(0.000650f, 0.001881f, 0.000085f);
		OtherAbsorptionScale = OtherAbsorptionRaw.y;
		OtherAbsorption = OtherAbsorptionRaw * (1.0f / OtherAbsorptionRaw.y);
		OtherTentDistribution.TipAltitude = 25.0f;
		OtherTentDistribution.TipValue = 1.0f;
		OtherTentDistribution.Width = 15.0f;

		SkyLuminanceFactor = Vector3f(1.0f, 1.0f, 1.0f);
		MultiScatteringFactor = 1.0f;
		AerialPespectiveViewDistanceScale = 1.0f;
		HeightFogContribution = 1.0f;
		TransmittanceMinLightElevationAngle = -90.0f;
		AerialPerspectiveStartDepth = 0.1f;
		AerialPerspectiveVolumeDepth = 96.0f;

		TraceSampleCountScale = 1.0f;
	}

	bool Sky::Init()
	{
		return true;
	}

	void Sky::PrepareView(View* view)
	{
		ViewRenderData& prepareData = view->GetPrepareData();

		//prepareData.m_UpdateSkyLut = (ElapsedTimer::FrameIndex() % 10) == 1; // every 10 frame for now.

		// Convert Tent distribution to linear curve coefficients.
		auto TentToCoefficients = [](const TentDistribution& Tent, float& LayerWidth, float& LinTerm0, float& LinTerm1, float& ConstTerm0, float& ConstTerm1)
			{
				if (Tent.Width > 0.0f && Tent.TipValue > 0.0f)
				{
					const float px = Tent.TipAltitude;
					const float py = Tent.TipValue;
					const float slope = Tent.TipValue / Tent.Width;
					LayerWidth = px;
					LinTerm0 = slope;
					LinTerm1 = -slope;
					ConstTerm0 = py - px * LinTerm0;
					ConstTerm1 = py - px * LinTerm1;
				}
				else
				{
					LayerWidth = 0.0f;
					LinTerm0 = 0.0f;
					LinTerm1 = 0.0f;
					ConstTerm0 = 0.0f;
					ConstTerm1 = 0.0f;
				}
			};

		prepareData.m_AtmosphereData.BottomRadiusKm = m_AtmosphereParams.BottomRadius;
		prepareData.m_AtmosphereData.TopRadiusKm = m_AtmosphereParams.BottomRadius + std::max(0.1f, m_AtmosphereParams.AtmosphereHeight);

		prepareData.m_AtmosphereData.GroundAlbedo = vkr::DecodeColor(m_AtmosphereParams.GroundAlbedo,ColorSpace::sRGB);
		prepareData.m_AtmosphereData.MultiScatteringFactor = std::clamp(m_AtmosphereParams.MultiScatteringFactor, 0.0f, 100.0f);

		auto ConvertCoefficientsFromSRGBToWorkingColorSpace = [](Vector3f CoeffSRGBLinear)
			{
				const ColorSpace& WorkingColorSpace = ColorSpace::DefaultSpace();
				if (WorkingColorSpace.m_Gamut.m_Type == ColorGamutType::COLOR_GAMUT_TYPE_SRGB) //How can I check if we are working in sRGB
				{
					return CoeffSRGBLinear;
				}
				else
				{
					// Compute the transmittance color from the coefficients.
					Vector3f Transmittance = Vector3f(
						std::exp(-CoeffSRGBLinear.x),
						std::exp(-CoeffSRGBLinear.y),
						std::exp(-CoeffSRGBLinear.z));

					// Convert transmittance color from sRGB to working color space.
					Transmittance = TransformColor(Transmittance, ColorSpace::sRGB, WorkingColorSpace); //Is this correct? We want linear to linear

					// New we have a transmittance in working color space, convert it back to coefficients for this working color space.
					return Vector3f(
						-std::log(std::max(0.00001f, Transmittance.x)),
						-std::log(std::max(0.00001f, Transmittance.y)),
						-std::log(std::max(0.00001f, Transmittance.z)));
				}
			};

		// Rayleigh scattering
		{
			prepareData.m_AtmosphereData.RayleighScattering = Clamp(m_AtmosphereParams.RayleighScattering * m_AtmosphereParams.RayleighScatteringScale,0.0f, 1e38f);
			prepareData.m_AtmosphereData.RayleighScattering = ConvertCoefficientsFromSRGBToWorkingColorSpace(prepareData.m_AtmosphereData.RayleighScattering);

			prepareData.m_AtmosphereData.RayleighDensityExpScale = -1.0f / m_AtmosphereParams.RayleighExponentialDistribution;
		}

		// Mie scattering
		{

			prepareData.m_AtmosphereData.MieScattering = Clamp(m_AtmosphereParams.MieScattering * m_AtmosphereParams.MieScatteringScale,0.0f, 1e38f);
			prepareData.m_AtmosphereData.MieScattering = ConvertCoefficientsFromSRGBToWorkingColorSpace(prepareData.m_AtmosphereData.MieScattering);

			prepareData.m_AtmosphereData.MieAbsorption = Clamp(m_AtmosphereParams.MieAbsorption * m_AtmosphereParams.MieAbsorptionScale, 0.0f, 1e38f);
			prepareData.m_AtmosphereData.MieAbsorption = ConvertCoefficientsFromSRGBToWorkingColorSpace(prepareData.m_AtmosphereData.MieAbsorption);

			prepareData.m_AtmosphereData.MieExtinction = prepareData.m_AtmosphereData.MieScattering + prepareData.m_AtmosphereData.MieAbsorption;
			prepareData.m_AtmosphereData.MiePhaseG = m_AtmosphereParams.MieAnisotropy;
			prepareData.m_AtmosphereData.MieDensityExpScale = -1.0f / m_AtmosphereParams.MieExponentialDistribution;
		}

		// Ozone
		{
			prepareData.m_AtmosphereData.AbsorptionExtinction = Clamp(m_AtmosphereParams.OtherAbsorption * m_AtmosphereParams.OtherAbsorptionScale, 0.0f, 1e38f);
			prepareData.m_AtmosphereData.AbsorptionExtinction = ConvertCoefficientsFromSRGBToWorkingColorSpace(prepareData.m_AtmosphereData.AbsorptionExtinction);

			TentToCoefficients(m_AtmosphereParams.OtherTentDistribution, 
				prepareData.m_AtmosphereData.AbsorptionDensity0LayerWidth,
				prepareData.m_AtmosphereData.AbsorptionDensity0LinearTerm,
				prepareData.m_AtmosphereData.AbsorptionDensity1LinearTerm,
				prepareData.m_AtmosphereData.AbsorptionDensity0ConstantTerm,
				prepareData.m_AtmosphereData.AbsorptionDensity1ConstantTerm);
		}

		//prepareData.m_AtmosphereData.TransmittanceMinLightElevationAngle = m_AtmosphereParams.TransmittanceMinLightElevationAngle;

		// Where are we in relation to our "virtual planet"? For atmosphere calculations we need to know where the observer/camera sits within the atmosphere
		// Were gonna consider a flat world in our scene and that that Y = 0 is where our earth surface is
		// The constants below should match the one in SkyAtmosphereCommon.ush
		Vector3 CameraWorldPos = Vector3f(prepareData.m_CameraData.CameraWorldMatrix[9], prepareData.m_CameraData.CameraWorldMatrix[10], prepareData.m_CameraData.CameraWorldMatrix[11]) * VkrUnitsToSkyUnits;
		//This will consider the planet center always directly under the camera (flat world)
		Vector3f PlanetCenterWorld =  Vector3f(CameraWorldPos.x, -(m_AtmosphereParams.XZPlaneDatum + prepareData.m_AtmosphereData.BottomRadiusKm), CameraWorldPos.z);
		Vector3f PlanetCenterTranslatedWorld = PlanetCenterWorld - CameraWorldPos; //Where the planet center is relative to the camera

		// 		const float Offset = PlanetRadiusOffset * SkyUnitToCm;
		// 		const float BottomRadiusWorld = BottomRadiusKm * SkyUnitToCm;
		// 		const Vector3f PlanetCenterWorld = PlanetCenterKm * SkyUnitToCm;
		// 		const Vector3f PlanetCenterTranslatedWorld = PlanetCenterWorld + PreViewTranslation;
		// 		const Vector3f WorldCameraOriginTranslatedWorld = WorldCameraOrigin + PreViewTranslation;
		// 		const Vector3f PlanetCenterToCameraTranslatedWorld = WorldCameraOriginTranslatedWorld - PlanetCenterTranslatedWorld;
		// 		const float DistanceToPlanetCenterTranslatedWorld = vkr::Length(PlanetCenterToCameraTranslatedWorld);
		// 		// If the camera is below the planet surface, we snap it back onto the surface.
		// 		// This is to make sure the sky is always visible even if the camera is inside the virtual planet.
		// 		Vector3f SkyCameraTranslatedWorldOriginTranslatedWorld = Vector3f(
		// 			DistanceToPlanetCenterTranslatedWorld < (BottomRadiusWorld + Offset) ?
		// 			PlanetCenterTranslatedWorld + (BottomRadiusWorld + Offset) * (PlanetCenterToCameraTranslatedWorld / DistanceToPlanetCenterTranslatedWorld) :
		// 			WorldCameraOriginTranslatedWorld);

		prepareData.m_SkyData.SkyPlanetTranslatedWorldCenterAndViewHeight = Vector4f(PlanetCenterTranslatedWorld.x,
			PlanetCenterTranslatedWorld.y,
			PlanetCenterTranslatedWorld.z,
			Length(PlanetCenterTranslatedWorld));

		// Compute the basis vectors for the frame of reference of the sky view LUT. This is a frame of reference tangent to the earth surface at the point the camera is
		// Our world is flat and not curved so we consider +Y  the up vector
		Mat44 SkyViewLutReferential = Mat44::Identity();
		Vector3f ViewForward = Vector3f(prepareData.m_CameraData.CameraWorldMatrix[6], prepareData.m_CameraData.CameraWorldMatrix[7], prepareData.m_CameraData.CameraWorldMatrix[8]);
		Vector3f ViewRight = Vector3f(prepareData.m_CameraData.CameraWorldMatrix[0], prepareData.m_CameraData.CameraWorldMatrix[1], prepareData.m_CameraData.CameraWorldMatrix[2]);
		
		Vector3f Up = CameraWorldPos - PlanetCenterWorld;
		vkr::Normalize(Up);
		Vector3f Forward = ViewForward;		// This can make texel visible when the camera is rotating. Use constant world direction instead?
		//FVector3f	Left = normalize(cross(Forward, Up)); 
		Vector3f	Left;
		Left = vkr::Cross(Forward, Up);
		vkr::Normalize(Left);
		const float DotMainDir = abs(vkr::Dot(Up, Forward));
		if (DotMainDir > 0.999f)
		{
			// When it becomes hard to generate a referential, generate it procedurally.
			// [ Duff et al. 2017, "Building an Orthonormal Basis, Revisited" ]
			const float Sign = Up.z >= 0.0f ? 1.0f : -1.0f;
			const float a = -1.0f / (Sign + Up.z);
			const float b = Up.x * Up.y * a;
			Forward = Vector3f(1 + Sign * a * pow(Up.x, 2.0f), Sign * b, -Sign * Up.x);
			Left = Vector3f(b, Sign + a * pow(Up.y, 2.0f), -Up.y);

			SkyViewLutReferential = vkr::Compose(
				Vector4f(Forward.x, Forward.y, Forward.z, 0),
				Vector4f(Left.x, Left.y, Left.z, 0),
				Vector4f(Up.x, Up.y, Up.z, 0),
				Vector4f(0, 0, 0, 1)
			);
			SkyViewLutReferential = SkyViewLutReferential.GetTransposed();
		}
		else
		{
			// This is better as it should be more stable with respect to camera forward.
			Forward = vkr::Cross(Up, Left);
			vkr::Normalize(Forward);
			SkyViewLutReferential[0] = Forward.x; SkyViewLutReferential[1] = Forward.y; SkyViewLutReferential[2] = Forward.z;
			SkyViewLutReferential[4] = Left.x; SkyViewLutReferential[5] = Left.y; SkyViewLutReferential[6] = Left.z;
			SkyViewLutReferential[8] = Up.x; SkyViewLutReferential[9] = Up.y; SkyViewLutReferential[10] = Up.z;
			SkyViewLutReferential = SkyViewLutReferential.GetTransposed();
		}

		prepareData.m_SkyData.SkyViewLutReferential = SkyViewLutReferential;
		prepareData.m_SkyData.SkyViewLutSizeAndInvSize = Vector4f(SKYVIEW_TEXTURE_WIDTH, SKYVIEW_TEXTURE_HEIGHT, 1.0f / SKYVIEW_TEXTURE_WIDTH, 1.0f / SKYVIEW_TEXTURE_HEIGHT);

		prepareData.m_SkyData.FogShowFlagFactor = m_AtmosphereParams.EnableAerialPerspective ? 1.0 : 0.0f;
		prepareData.m_SkyData.AerialPerspectiveStartDepthKm = m_AtmosphereParams.AerialPerspectiveStartDepth;
		prepareData.m_SkyData.AerialPerspectiveLutSizeAndInvSize = Vector4f(AERIAL_PERSPECTIVE_TEXTURE_WIDTHHEIGHT, AERIAL_PERSPECTIVE_TEXTURE_WIDTHHEIGHT, 1.0f / AERIAL_PERSPECTIVE_TEXTURE_WIDTHHEIGHT, 1.0f / AERIAL_PERSPECTIVE_TEXTURE_WIDTHHEIGHT);
		prepareData.m_SkyData.AerialPerspectiveLutDepthResolution = AERIAL_PERSPECTIVE_TEXTURE_DEPTH;
		// TODO: Add other sky related prepare data here
	}

	bool SkyRenderer::Init()
	{
		Render::Device* device = Render::GetDevice();

		m_SkyTransmittanceLUTComputeShader = device->CreateShader(SystemPaths::GetInContentDirectory(CONTENT_DIRECTORY_ENGINE, "shaders/skytransmittancelut.hlsl"), L"MainCS", vkr::Render::SHADER_STAGE_COMPUTE);
		Render::PipelineStateDesc skyTransmittanceLUTPSODesc = Render::PipelineStateDesc(Render::PIPELINE_STATE_TYPE_COMPUTE);
		skyTransmittanceLUTPSODesc.Compute.m_ComputeShader = m_SkyTransmittanceLUTComputeShader.get();
		m_SkyTransmittanceLUTPSO = device->CreatePipelineState(skyTransmittanceLUTPSODesc);

		m_SkyMultiScatterLUTComputeShader = device->CreateShader(SystemPaths::GetInContentDirectory(CONTENT_DIRECTORY_ENGINE, "shaders/skymultiscatteringlut.hlsl"), L"MainCS", vkr::Render::SHADER_STAGE_COMPUTE);
		Render::PipelineStateDesc skyMultiscatterLUTPSODesc = Render::PipelineStateDesc(Render::PIPELINE_STATE_TYPE_COMPUTE);
		skyMultiscatterLUTPSODesc.Compute.m_ComputeShader = m_SkyMultiScatterLUTComputeShader.get();
		m_SkyMultiScatterLUTPSO = device->CreatePipelineState(skyMultiscatterLUTPSODesc);

		m_SkyViewLUTComputeShader = device->CreateShader(SystemPaths::GetInContentDirectory(CONTENT_DIRECTORY_ENGINE, "shaders/skyviewlut.hlsl"), L"MainCS", vkr::Render::SHADER_STAGE_COMPUTE);
		Render::PipelineStateDesc skyViewLUTPSODesc = Render::PipelineStateDesc(Render::PIPELINE_STATE_TYPE_COMPUTE);
		skyViewLUTPSODesc.Compute.m_ComputeShader = m_SkyViewLUTComputeShader.get();
		m_SkyViewLUTPSO = device->CreatePipelineState(skyViewLUTPSODesc);

		m_SkyAerialPerspectiveLUTComputeShader = device->CreateShader(SystemPaths::GetInContentDirectory(CONTENT_DIRECTORY_ENGINE, "shaders/skyaerialperspectivelut.hlsl"), L"MainCS", vkr::Render::SHADER_STAGE_COMPUTE);
		Render::PipelineStateDesc skyAerialPerspectiveLUTPSODesc = Render::PipelineStateDesc(Render::PIPELINE_STATE_TYPE_COMPUTE);
		skyAerialPerspectiveLUTPSODesc.Compute.m_ComputeShader = m_SkyAerialPerspectiveLUTComputeShader.get();
		m_SkyAerialPerspectiveLUTPSO = device->CreatePipelineState(skyAerialPerspectiveLUTPSODesc);

// 		m_SkyAerialPerspectiveShader = device->CreateShader(SystemPaths::GetInContentDirectory(CONTENT_DIRECTORY_ENGINE, "shaders/skyaerialperspective.hlsl"), L"Main", vkr::Render::SHADER_STAGE_PIXEL);
// 		Render::PipelineStateDesc skyAerialPerspectivePSODesc = Render::PipelineStateDesc(Render::PIPELINE_STATE_TYPE_DEFAULT);
// 		skyAerialPerspectivePSODesc.Default.m_VertexShader = m_SkyAerialPerspectiveShader.get();
// 		skyAerialPerspectivePSODesc.Default.m_PixelShader = m_SkyAerialPerspectiveShader.get();
// 		m_SkyAerialPerspectivePSO = device->CreatePipelineState(skyAerialPerspectivePSODesc);

		return true;
	}

	void SkyRenderer::ComputeLuts(View* view)
	{
		const ViewRenderData& renderData = view->GetRenderData();
		ViewRenderTargets& renderTargets = view->GetRenderTargets();
		Render::Context* ctx = Render::Context::GetCurrentContext();

		ctx->ClearStateCache(); //Maybe we should do this somewhere else 

		renderTargets.m_SkyTransmittanceLUT.m_IsWritable = true;
		renderTargets.m_SkyTransmittanceLUT.m_Format = Render::FORMAT_RGBA32_FLOAT;
		renderTargets.m_SkyTransmittanceLUT.Update(Vector2u(TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT), "TransmittanceLUT");

 		renderTargets.m_SkyMultiScatteringLUT.m_IsWritable = true;
 		renderTargets.m_SkyMultiScatteringLUT.m_Format = Render::FORMAT_RGBA32_FLOAT;
 		renderTargets.m_SkyMultiScatteringLUT.Update(Vector2u(MULTISCATTERING_TEXTURE_WIDTH, MULTISCATTERING_TEXTURE_HEIGHT), "MultiScatteringLUT");
 
		renderTargets.m_SkyViewLUT.m_IsWritable = true;
		renderTargets.m_SkyViewLUT.m_Format = Render::FORMAT_RGBA32_FLOAT;
		renderTargets.m_SkyViewLUT.Update(Vector2u(SKYVIEW_TEXTURE_WIDTH, SKYVIEW_TEXTURE_HEIGHT), "SkyViewLUT");
 
 		renderTargets.m_SkyAerialPerspectiveLUT.m_IsWritable = true;
 		renderTargets.m_SkyAerialPerspectiveLUT.m_Format = Render::FORMAT_RGBA32_FLOAT;
 		renderTargets.m_SkyAerialPerspectiveLUT.Update(Vector3u(AERIAL_PERSPECTIVE_TEXTURE_WIDTHHEIGHT, AERIAL_PERSPECTIVE_TEXTURE_WIDTHHEIGHT, AERIAL_PERSPECTIVE_TEXTURE_DEPTH), "AerialPerspectiveCameraVolume");

		VKR_CONTEXT_EVENT_FUNCTION(ctx);

		//Transmittance
		{
			std::vector<Render::TextureBarrierDesc> barriers;
			{
				//Transition transmittance LUT to write
				Render::TextureBarrierDesc barrierDesc;
				barrierDesc.m_Texture = renderTargets.m_SkyTransmittanceLUT.m_Texture.get();
				barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_COMPUTE;
				barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_WRITE;
				barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_READ_WRITE_RESOURCE;
				barriers.push_back(barrierDesc);
			}
			ctx->TextureBarrier(barriers.size(), barriers.data());
		}

		ctx->BindPipelineState(m_SkyTransmittanceLUTPSO.get());

		struct alignas(16) TransmittanceConstantData
		{
			vkr::Graphics::AtmosphereData atmosphere;
			Vector4f transmittanceLutSizeAndInvSize;
			uint32_t transmittanceTextureDescriptorIndex;
		};

		TransmittanceConstantData constantData;
		constantData.atmosphere = renderData.m_AtmosphereData;
		constantData.transmittanceLutSizeAndInvSize = Vector4f(TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT, 1.0f/TRANSMITTANCE_TEXTURE_WIDTH, 1.0f/TRANSMITTANCE_TEXTURE_HEIGHT);
		constantData.transmittanceTextureDescriptorIndex = renderTargets.m_SkyTransmittanceLUT.m_TextureViewRW->GetIndex();

		ctx->BindLocalConstantBuffer(sizeof(TransmittanceConstantData), &constantData, 0);

		ctx->DispatchThreads(Vector3u(TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT, 1));

		//Multiscattering
		{
			std::vector<Render::TextureBarrierDesc> barriers;
			{
				//Transition multiscattering LUT to write
				Render::TextureBarrierDesc barrierDesc;
				barrierDesc.m_Texture = renderTargets.m_SkyMultiScatteringLUT.m_Texture.get();
				barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_COMPUTE;
				barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_WRITE;
				barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_READ_WRITE_RESOURCE;
				barriers.push_back(barrierDesc);

				//Transition transmittance LUT to read
				Render::TextureBarrierDesc transmittanceBarrierDesc;
				transmittanceBarrierDesc.m_Texture = renderTargets.m_SkyTransmittanceLUT.m_Texture.get();
				transmittanceBarrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_COMPUTE;
				transmittanceBarrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_READ;
				transmittanceBarrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_READ_RESOURCE;
				barriers.push_back(transmittanceBarrierDesc);
			}
			ctx->TextureBarrier(barriers.size(), barriers.data());
		}

		ctx->BindPipelineState(m_SkyMultiScatterLUTPSO.get());

		struct alignas(16) MultiScatteringConstantData
		{
			vkr::Graphics::AtmosphereData atmosphere;
			Vector4f multiscatteringLutSizeAndInvSize;
			uint32_t transmittanceTextureDescriptorIndex;
			uint32_t multiscatteringTextureDescriptorIndex;
			uint32_t _pad[2];
		};

		MultiScatteringConstantData multiscatteringConstantData;
		multiscatteringConstantData.atmosphere = renderData.m_AtmosphereData;
		multiscatteringConstantData.multiscatteringLutSizeAndInvSize = Vector4f(MULTISCATTERING_TEXTURE_WIDTH, MULTISCATTERING_TEXTURE_HEIGHT, 1.0f / MULTISCATTERING_TEXTURE_WIDTH, 1.0f / MULTISCATTERING_TEXTURE_HEIGHT);
		multiscatteringConstantData.transmittanceTextureDescriptorIndex = renderTargets.m_SkyTransmittanceLUT.m_TextureView->GetIndex();
		multiscatteringConstantData.multiscatteringTextureDescriptorIndex = renderTargets.m_SkyMultiScatteringLUT.m_TextureViewRW->GetIndex();

		ctx->BindLocalConstantBuffer(sizeof(MultiScatteringConstantData), &multiscatteringConstantData, 0);

		ctx->DispatchThreads(Vector3u(MULTISCATTERING_TEXTURE_WIDTH, MULTISCATTERING_TEXTURE_HEIGHT, 1));
		
		// Sky view
		{
			std::vector<Render::TextureBarrierDesc> barriers;
			
			//Transition sky view LUT to write
			Render::TextureBarrierDesc skyViewBarrierDesc;
			skyViewBarrierDesc.m_Texture = renderTargets.m_SkyViewLUT.m_Texture.get();
			skyViewBarrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_COMPUTE;
			skyViewBarrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_WRITE;
			skyViewBarrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_READ_WRITE_RESOURCE;
			barriers.push_back(skyViewBarrierDesc);

			//Transition multiscattering LUT to read
 			Render::TextureBarrierDesc multiScatteringBarrierDesc;
 			multiScatteringBarrierDesc.m_Texture = renderTargets.m_SkyMultiScatteringLUT.m_Texture.get();
 			multiScatteringBarrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_COMPUTE;
 			multiScatteringBarrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_READ;
 			multiScatteringBarrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_READ_RESOURCE;
 			barriers.push_back(multiScatteringBarrierDesc);
			ctx->TextureBarrier(barriers.size(), barriers.data());
		}

		ctx->BindPipelineState(m_SkyViewLUTPSO.get());

		struct alignas(16) SkyViewConstantData
		{
			vkr::Graphics::AtmosphereData atmosphere;
			Mat44 SkyViewLutReferential;
			Mat44 InvViewProjection;
			Vector4f SkyViewLutSizeAndInvSize;
			Vector4f SkyPlanetTranslatedWorldCenterAndViewHeight;
			Vector3f AtmosphereLightDirection0;
			uint32_t _pad0;
			Vector3f AtmosphereLightIlluminanceOuterSpace0;
			uint32_t _pad1;
			Vector3f AtmosphereLightDirection1;
			uint32_t _pad2;
			Vector3f AtmosphereLightIlluminanceOuterSpace1;
			uint32_t TransmittanceTextureDescriptorIndex;
			uint32_t MultiScatteringTextureDescriptorIndex;
			uint32_t SkyViewTextureDescriptorIndex;
			uint32_t _pad3[2];
		};
		
		SkyViewConstantData skyViewConstantData;
		skyViewConstantData.atmosphere = renderData.m_AtmosphereData;
		skyViewConstantData.MultiScatteringTextureDescriptorIndex = renderTargets.m_SkyMultiScatteringLUT.m_TextureView->GetIndex();
		skyViewConstantData.TransmittanceTextureDescriptorIndex = renderTargets.m_SkyTransmittanceLUT.m_TextureView->GetIndex();
		skyViewConstantData.SkyViewTextureDescriptorIndex = renderTargets.m_SkyViewLUT.m_TextureViewRW->GetIndex();
		skyViewConstantData.SkyViewLutSizeAndInvSize = renderData.m_SkyData.SkyViewLutSizeAndInvSize;

		skyViewConstantData.SkyViewLutReferential = renderData.m_SkyData.SkyViewLutReferential;
		skyViewConstantData.SkyPlanetTranslatedWorldCenterAndViewHeight = renderData.m_SkyData.SkyPlanetTranslatedWorldCenterAndViewHeight;
		skyViewConstantData.AtmosphereLightDirection0 = renderData.m_DirectionalLights[0].Direction;
		skyViewConstantData.AtmosphereLightIlluminanceOuterSpace0 = renderData.m_DirectionalLights[0].Emission;
		skyViewConstantData.AtmosphereLightDirection1 = renderData.m_DirectionalLights[1].Direction;
		skyViewConstantData.AtmosphereLightIlluminanceOuterSpace1 = renderData.m_DirectionalLights[1].Emission;
		skyViewConstantData.InvViewProjection = renderData.m_CameraData.InvViewProjectionMatrix;

		ctx->BindLocalConstantBuffer(sizeof(SkyViewConstantData), &skyViewConstantData, 0);

		ctx->DispatchThreads(Vector3u(SKYVIEW_TEXTURE_WIDTH, SKYVIEW_TEXTURE_HEIGHT, 1));

		//Aerial perspective
		{
			std::vector<Render::TextureBarrierDesc> barriers;
			{
				//Transition aerial perspective LUT to write
				Render::TextureBarrierDesc barrierDesc;
				barrierDesc.m_Texture = renderTargets.m_SkyAerialPerspectiveLUT.m_Texture.get();
				barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_COMPUTE;
				barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_WRITE;
				barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_READ_WRITE_RESOURCE;
				barriers.push_back(barrierDesc);
			}
			ctx->TextureBarrier(barriers.size(), barriers.data());
		}

		ctx->BindPipelineState(m_SkyAerialPerspectiveLUTPSO.get());

		struct alignas(16) AerialPerspectiveLUTConstantData
		{
			vkr::Graphics::AtmosphereData Atmosphere;

			Mat44 InvViewProjection;

			Vector4f AerialPerspectiveLutSizeAndInvSize;

			Vector4f SkyPlanetTranslatedWorldCenterAndViewHeight;

			Vector4f CameraPosition;

			Vector4f ViewSizeAndInvSize;

			Vector3f AtmosphereLightDirection0;
			uint32_t _pad0;

			Vector3f AtmosphereLightIlluminanceOuterSpace0;
			uint32_t _pad1;

			Vector3f AtmosphereLightDirection1;
			uint32_t _pad2;

			Vector3f AtmosphereLightIlluminanceOuterSpace1;
			uint32_t _pad3;

			float FogShowFlagFactor;
			Vector2f AerialPerspectiveLutDepthAndInvDepth;
			float AerialPerspectiveStartDepthKm;

			float CameraAerialPerspectiveVolumeDepthSliceLengthKm;
			uint32_t TransmittanceLUTTextureDescriptorIndex;
			uint32_t MultiscatterLUTTextureDescriptorIndex;
			uint32_t AerialPerspectiveTextureDescriptorIndex;
		};

		AerialPerspectiveLUTConstantData aerialPerspectiveLUTConstantData;
		aerialPerspectiveLUTConstantData.Atmosphere = renderData.m_AtmosphereData;
		aerialPerspectiveLUTConstantData.InvViewProjection = renderData.m_CameraData.InvViewProjectionMatrix;
		aerialPerspectiveLUTConstantData.AerialPerspectiveLutSizeAndInvSize = Vector4f(AERIAL_PERSPECTIVE_TEXTURE_WIDTHHEIGHT, AERIAL_PERSPECTIVE_TEXTURE_WIDTHHEIGHT, 1.0f / AERIAL_PERSPECTIVE_TEXTURE_WIDTHHEIGHT, 1.0f / AERIAL_PERSPECTIVE_TEXTURE_WIDTHHEIGHT);
		aerialPerspectiveLUTConstantData.SkyPlanetTranslatedWorldCenterAndViewHeight = renderData.m_SkyData.SkyPlanetTranslatedWorldCenterAndViewHeight;
		aerialPerspectiveLUTConstantData.CameraPosition = Vector4f(renderData.m_CameraData.CameraWorldMatrix[9], renderData.m_CameraData.CameraWorldMatrix[10], renderData.m_CameraData.CameraWorldMatrix[11],1.0f);
		aerialPerspectiveLUTConstantData.ViewSizeAndInvSize = Vector4f(renderData.m_RenderSize.x, renderData.m_RenderSize.y, 1.0f / renderData.m_RenderSize.x, 1.0f / renderData.m_RenderSize.y);
		aerialPerspectiveLUTConstantData.AtmosphereLightDirection0 = renderData.m_DirectionalLights[0].Direction;
		aerialPerspectiveLUTConstantData.AtmosphereLightIlluminanceOuterSpace0 = renderData.m_DirectionalLights[0].Emission;
		aerialPerspectiveLUTConstantData.AtmosphereLightDirection1 = renderData.m_DirectionalLights[1].Direction;
		aerialPerspectiveLUTConstantData.AtmosphereLightIlluminanceOuterSpace1 = renderData.m_DirectionalLights[1].Emission;
		aerialPerspectiveLUTConstantData.FogShowFlagFactor = renderData.m_SkyData.FogShowFlagFactor;
		aerialPerspectiveLUTConstantData.AerialPerspectiveLutDepthAndInvDepth = Vector2f((float)AERIAL_PERSPECTIVE_TEXTURE_DEPTH, 1.0f / AERIAL_PERSPECTIVE_TEXTURE_DEPTH);
		aerialPerspectiveLUTConstantData.AerialPerspectiveStartDepthKm = renderData.m_SkyData.AerialPerspectiveStartDepthKm;
		aerialPerspectiveLUTConstantData.CameraAerialPerspectiveVolumeDepthSliceLengthKm = renderData.m_SkyData.AerialPerspectiveVolumeDepthKm / renderData.m_SkyData.AerialPerspectiveLutDepthResolution;
		aerialPerspectiveLUTConstantData.TransmittanceLUTTextureDescriptorIndex = renderTargets.m_SkyTransmittanceLUT.m_TextureView->GetIndex();
		aerialPerspectiveLUTConstantData.MultiscatterLUTTextureDescriptorIndex = renderTargets.m_SkyMultiScatteringLUT.m_TextureView->GetIndex();
		aerialPerspectiveLUTConstantData.AerialPerspectiveTextureDescriptorIndex = renderTargets.m_SkyAerialPerspectiveLUT.m_TextureViewRW->GetIndex();

		ctx->BindLocalConstantBuffer(sizeof(AerialPerspectiveLUTConstantData), &aerialPerspectiveLUTConstantData, 0);

		ctx->DispatchThreads(Vector3u(AERIAL_PERSPECTIVE_TEXTURE_WIDTHHEIGHT, AERIAL_PERSPECTIVE_TEXTURE_WIDTHHEIGHT, AERIAL_PERSPECTIVE_TEXTURE_DEPTH));
	}

	void SkyRenderer::ApplyAerialPerspective(View* view)
	{
		const ViewRenderData& renderData = view->GetRenderData();
		ViewRenderTargets& renderTargets = view->GetRenderTargets();
		Render::Context* ctx = Render::Context::GetCurrentContext();

		VKR_CONTEXT_EVENT_FUNCTION(ctx);
	}
	

}
