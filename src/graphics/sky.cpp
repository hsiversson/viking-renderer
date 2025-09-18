#include "sky.h"

#include "core/common.h"
#include "render/profiler.h"
#include "view.h"
#include "viewrenderdata.h"

using namespace vkr;

namespace
{
	struct DensityProfileLayer
	{
		float width;
		float exp_term;
		float exp_scale;
		float linear_term;
		float constant_term;
		Vector3f _padding;
	};

	// An atmosphere density profile made of several layers on top of each other
	// (from bottom to top). The width of the last layer is ignored, i.e. it always
	// extend to the top atmosphere boundary. The profile values vary between 0
	// (null density) to 1 (maximum density).
	struct DensityProfile
	{
		DensityProfileLayer layers[2];
	};

	//Construct the per scene constant buffer
	struct alignas(16) AtmosphereData
	{
		// The solar irradiance at the top of the atmosphere.
		Vector3f solar_irradiance;
		// The sun's angular radius. Warning: the implementation uses approximations
		// that are valid only if this angle is smaller than 0.1 radians.
		float sun_angular_radius;

		// The distance between the planet center and the bottom of the atmosphere.
		float bottom_radius;
		// The distance between the planet center and the top of the atmosphere.
		float top_radius;
		Vector2f _pad1;

		// The density profile of air molecules, i.e. a function from altitude to
		// dimensionless values between 0 (null density) and 1 (maximum density).
		DensityProfile rayleigh_density;
		// The scattering coefficient of air molecules at the altitude where their
		// density is maximum (usually the bottom of the atmosphere), as a function of
		// wavelength. The scattering coefficient at altitude h is equal to
		// 'rayleigh_scattering' times 'rayleigh_density' at this altitude.
		Vector3f rayleigh_scattering;
		float _pad2;

		// The density profile of aerosols, i.e. a function from altitude to
		// dimensionless values between 0 (null density) and 1 (maximum density).
		DensityProfile mie_density;
		// The scattering coefficient of aerosols at the altitude where their density
		// is maximum (usually the bottom of the atmosphere), as a function of
		// wavelength. The scattering coefficient at altitude h is equal to
		// 'mie_scattering' times 'mie_density' at this altitude.
		Vector3f mie_scattering;
		float _pad3;

		// The extinction coefficient of aerosols at the altitude where their density
		// is maximum (usually the bottom of the atmosphere), as a function of
		// wavelength. The extinction coefficient at altitude h is equal to
		// 'mie_extinction' times 'mie_density' at this altitude.
		Vector3f mie_extinction;
		// The asymetry parameter for the Cornette-Shanks phase function for the
		// aerosols.
		float mie_phase_function_g;

		// The density profile of air molecules that absorb light (e.g. ozone), i.e.
		// a function from altitude to dimensionless values between 0 (null density)
		// and 1 (maximum density).
		DensityProfile absorption_density;
		// The extinction coefficient of molecules that absorb light (e.g. ozone) at
		// the altitude where their density is maximum, as a function of wavelength.
		// The extinction coefficient at altitude h is equal to
		// 'absorption_extinction' times 'absorption_density' at this altitude.
		Vector3f absorption_extinction;
		float _pad4;

		// The average albedo of the ground.
		Vector3f ground_albedo;
		// The cosine of the maximum Sun zenith angle for which atmospheric scattering
		// must be precomputed (for maximum precision, use the smallest Sun zenith
		// angle yielding negligible sky light radiance values. For instance, for the
		// Earth case, 102 degrees is a good choice - yielding mu_s_min = -0.2).
		float mu_s_min;
	};

	struct alignas(16) TransmittanceConstantData
	{
		AtmosphereData atmosphere;
		Vector2u transmittanceTextureSize;
		uint32_t transmittanceTextureDescriptorIndex;
	};

	struct alignas(16) SkyViewConstantData
	{
		uint32_t transmittanceTextureDesciptorIndex;
		uint32_t multiscatteringTextureDescriptorIndex;
		uint32_t skyViewTextureDescriptorIndex;
		uint32_t _pad0;
		Vector4f skyViewLUTSizeAndInvSize;
	};

	constexpr uint32_t TRANSMITTANCE_TEXTURE_WIDTH = 256;
	constexpr uint32_t TRANSMITTANCE_TEXTURE_HEIGHT = 64;

	constexpr uint32_t MULTISCATTERING_TEXTURE_WIDTH = 32;
	constexpr uint32_t MULTISCATTERING_TEXTURE_HEIGHT = 32;

	constexpr uint32_t SKYVIEW_TEXTURE_WIDTH = 192;
	constexpr uint32_t SKYVIEW_TEXTURE_HEIGHT = 104;

	constexpr uint32_t AERIAL_PERSPECTIVE_TEXTURE_WIDTH = 32;
	constexpr uint32_t AERIAL_PERSPECTIVE_TEXTURE_HEIGHT = 32;
	constexpr uint32_t AERIAL_PERSPECTIVE_TEXTURE_DEPTH = 16;

	constexpr float LambdaMin = 360;
	constexpr float LambdaR = 680;
	constexpr float LambdaG = 510;
	constexpr float LambdaB = 440;

	constexpr double OzoneCrossSection[48] = {
	1.18e-27, 2.182e-28, 2.818e-28, 6.636e-28, 1.527e-27, 2.763e-27, 5.52e-27,
	8.451e-27, 1.582e-26, 2.316e-26, 3.669e-26, 4.924e-26, 7.752e-26, 9.016e-26,
	1.48e-25, 1.602e-25, 2.139e-25, 2.755e-25, 3.091e-25, 3.5e-25, 4.266e-25,
	4.672e-25, 4.398e-25, 4.701e-25, 5.019e-25, 4.305e-25, 3.74e-25, 3.215e-25,
	2.662e-25, 2.238e-25, 1.852e-25, 1.473e-25, 1.209e-25, 9.423e-26, 7.455e-26,
	6.566e-26, 5.105e-26, 4.15e-26, 4.228e-26, 3.237e-26, 2.451e-26, 2.801e-26,
	2.534e-26, 1.624e-26, 1.465e-26, 2.078e-26, 1.383e-26, 7.105e-27
	};

	constexpr float BottomRadius = 6360.0;
	constexpr float TopRadius = 6420.0;
	constexpr double Rayleigh = 1.24062e-6;
	constexpr float RayleighScaleHeight = 8000.0f;
	constexpr float MieScaleHeight = 1200.0f;
	constexpr float MiePhaseFunctionG = 0.8f;
	constexpr float MieSingleScatteringAlbedo = 0.9;
	constexpr double MieAngstromAlpha = 0.0;
	constexpr double MieAngstromBeta = 5.328e-3;
	constexpr float GroundAlbedo = 0.1f;
	constexpr float SunAngularRadius = 0.00935 / 2.0;
	constexpr float MaxSunZenithAngle = 120.0 / 180.0 * vkr::PI;
	constexpr DensityProfileLayer RayleighLayer = { 0.0, 1.0, -1.0 / RayleighScaleHeight, 0.0, 0.0 };
	constexpr DensityProfileLayer MieLayer(0.0, 1.0, -1.0 / MieScaleHeight, 0.0, 0.0);

	// From https://en.wikipedia.org/wiki/Dobson_unit, in molecules.m^-2.
	constexpr double DobsonUnit = 2.687e20;
	// Maximum number density of ozone molecules, in m^-3 (computed so at to get
	// 300 Dobson units of ozone - for this we divide 300 DU by the integral of
	// the ozone density profile defined below, which is equal to 15km).
	constexpr double MaxOzoneNumberDensity = 300.0 * DobsonUnit / 15000.0;

	AtmosphereData& GetAtmosphereData()
	{
		static bool isComputed = false;
		static AtmosphereData atmosphereData = {};
		if (!isComputed)
		{
			//atmosphereData.solar_irradiance = Vector3f(1.474, 1.8504, 1.91198); //Irradiance values in W.m^-2 for wavelengths R= 680 G = 550 B = 440. God help me if i understand this shit. Can we just use an RGB emission from the light directly??
			atmosphereData.solar_irradiance = { 1.0f, 1.0f, 1.0f };	// Using a normalized sun illuminance. This is to make sure the LUTs acts as a transfer factor to apply the runtime computed sun irradiance over.
			atmosphereData.sun_angular_radius = SunAngularRadius;
			atmosphereData.bottom_radius = BottomRadius;
			atmosphereData.top_radius = TopRadius;
			atmosphereData.rayleigh_density = { { {}, RayleighLayer } };
			atmosphereData.rayleigh_scattering = Rayleigh * Vector3f(pow(LambdaR, -4), pow(LambdaG, -4), pow(LambdaB, -4)) * 1000.0f;
			atmosphereData.mie_density = { {{}, MieLayer} };
			Vector3f mie = MieAngstromBeta / MieScaleHeight * Vector3f(pow(LambdaR, -MieAngstromAlpha), pow(LambdaG, -MieAngstromAlpha), pow(LambdaB, -MieAngstromAlpha));
			atmosphereData.mie_scattering = mie * MieSingleScatteringAlbedo * 1000.0f;
			atmosphereData.mie_extinction = mie * 1000.0f;
			atmosphereData.mie_phase_function_g = MiePhaseFunctionG;
			atmosphereData.absorption_density = { {{25000.0, 0.0, 0.0, 1.0 / 15000.0, -2.0 / 3.0}, {0.0, 0.0, 0.0, -1.0 / 15000.0, 8.0 / 3.0}} };
			atmosphereData.absorption_extinction = MaxOzoneNumberDensity * Vector3f(OzoneCrossSection[int(LambdaR - LambdaMin) / 10], OzoneCrossSection[int(LambdaG - LambdaMin) / 10], OzoneCrossSection[int(LambdaB - LambdaMin) / 10]) * 1000.0;
			atmosphereData.ground_albedo = Vector3f(0.1, 0.1, 0.1);
			atmosphereData.mu_s_min = cos(MaxSunZenithAngle);
		}
		return atmosphereData;
	}
}

namespace vkr::Graphics
{
	bool Sky::Init()
	{
		return true;
	}

	void Sky::PrepareView(View* view)
	{
		ViewRenderData& prepareData = view->GetPrepareData();

		prepareData.m_UpdateSkyLut = (ElapsedTimer::FrameIndex() % 10) == 0; // every 10 frame for now.

		// TODO: Add other sky related prepare data here
	}

	bool SkyRenderer::Init()
	{
		Render::Device* device = Render::GetDevice();

		m_SkyTransmittanceLUTComputeShader = device->CreateShader(SystemPaths::GetInContentDirectory(CONTENT_DIRECTORY_ENGINE, "shaders/skytransmittancelut.hlsl"), L"MainCS", vkr::Render::SHADER_STAGE_COMPUTE);
		Render::PipelineStateDesc skyTransmittanceLUTPSODesc = Render::PipelineStateDesc(Render::PIPELINE_STATE_TYPE_COMPUTE);
		skyTransmittanceLUTPSODesc.Compute.m_ComputeShader = m_SkyTransmittanceLUTComputeShader.get();
		m_SkyTransmittanceLUTPSO = device->CreatePipelineState(skyTransmittanceLUTPSODesc);

// 		m_SkyViewLUTComputeShader = device->CreateShader(SystemPaths::GetInContentDirectory(CONTENT_DIRECTORY_ENGINE, "shaders/skyviewlut.hlsl"), L"MainCS", vkr::Render::SHADER_STAGE_COMPUTE);
// 		Render::PipelineStateDesc skyViewLUTPSODesc = Render::PipelineStateDesc(Render::PIPELINE_STATE_TYPE_COMPUTE);
// 		skyViewLUTPSODesc.Compute.m_ComputeShader = m_SkyViewLUTComputeShader.get();
// 		m_SkyViewLUTPSO = device->CreatePipelineState(skyViewLUTPSODesc);

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

// 		renderTargets.m_SkyMultiScatteringLUT.m_IsWritable = true;
// 		renderTargets.m_SkyMultiScatteringLUT.m_Format = Render::FORMAT_RGBA32_FLOAT;
// 		renderTargets.m_SkyMultiScatteringLUT.Update(Vector2u(MULTISCATTERING_TEXTURE_WIDTH, MULTISCATTERING_TEXTURE_HEIGHT), "MultiScatteringLUT");
// 
// 		renderTargets.m_SkyViewLUT.m_IsWritable = true;
// 		renderTargets.m_SkyViewLUT.m_Format = Render::FORMAT_RGBA32_FLOAT;
// 		renderTargets.m_SkyViewLUT.Update(Vector2u(SKYVIEW_TEXTURE_WIDTH, SKYVIEW_TEXTURE_HEIGHT), "SkyViewLUT");
// 
// 		renderTargets.m_SkyAerialPerspective.m_IsWritable = true;
// 		renderTargets.m_SkyAerialPerspective.m_Format = Render::FORMAT_RGBA32_FLOAT;
// 		renderTargets.m_SkyAerialPerspective.Update(Vector3u(AERIAL_PERSPECTIVE_TEXTURE_WIDTH, AERIAL_PERSPECTIVE_TEXTURE_HEIGHT, AERIAL_PERSPECTIVE_TEXTURE_DEPTH), "AerialPerspectiveCameraVolume");

		VKR_CONTEXT_EVENT_FUNCTION(ctx);

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

		//Transmittance

		ctx->BindPipelineState(m_SkyTransmittanceLUTPSO.get());

		const AtmosphereData& atmosphereData = GetAtmosphereData();

		TransmittanceConstantData constantData;
		constantData.atmosphere = atmosphereData;
		constantData.transmittanceTextureSize = Vector2u(renderTargets.m_SkyTransmittanceLUT.m_Texture->m_TextureDesc.m_Size.x, renderTargets.m_SkyTransmittanceLUT.m_Texture->m_TextureDesc.m_Size.y);
		constantData.transmittanceTextureDescriptorIndex = renderTargets.m_SkyTransmittanceLUT.m_TextureViewRW->GetIndex();

		ctx->BindLocalConstantBuffer(sizeof(TransmittanceConstantData), &constantData, 0);

		ctx->DispatchThreads(Vector3u(constantData.transmittanceTextureSize.x, constantData.transmittanceTextureSize.y, 1));
		
		// Sky view
// 		{
// 			std::vector<Render::TextureBarrierDesc> barriers;
// 			
// 
// 			//Transition sky view LUT to write
// 			Render::TextureBarrierDesc barrierDesc;
// 			barrierDesc.m_Texture = renderTargets.m_SkyViewLUT.m_Texture.get();
// 			barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_COMPUTE;
// 			barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_WRITE;
// 			barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_READ_WRITE_RESOURCE;
// 			barriers.push_back(barrierDesc);
// 
// 			//Transition transmittance LUT to read
// 			Render::TextureBarrierDesc barrierDesc;
// 			barrierDesc.m_Texture = renderTargets.m_SkyTransmittanceLUT.m_Texture.get();
// 			barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_COMPUTE;
// 			barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_READ;
// 			barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_READ_RESOURCE;
// 			barriers.push_back(barrierDesc);
// 			ctx->TextureBarrier(barriers.size(), barriers.data());
// 
// 			//Transition multiscattering LUT to read
// 			Render::TextureBarrierDesc barrierDesc;
// 			barrierDesc.m_Texture = renderTargets.m_SkyMultiScatteringLUT.m_Texture.get();
// 			barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_COMPUTE;
// 			barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_READ;
// 			barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_READ_RESOURCE;
// 			barriers.push_back(barrierDesc);
// 			ctx->TextureBarrier(barriers.size(), barriers.data());
// 		}
// 
// 		ctx->BindPipelineState(m_SkyViewLUTPSO.get());
// 		SkyViewConstantData skyViewConstantData;
// 		skyViewConstantData.multiscatteringTextureDescriptorIndex = renderTargets.m_SkyMultiScatteringLUT.m_TextureView->GetIndex();
// 		skyViewConstantData.transmittanceTextureDesciptorIndex = renderTargets.m_SkyTransmittanceLUT.m_TextureView->GetIndex();
// 		skyViewConstantData.skyViewTextureDescriptorIndex = renderTargets.m_SkyViewLUT.m_TextureViewRW->GetIndex();
// 
// 		// Where are we in relation to our "virtual planet"? For atmosphere calculations we need to know where the observer/camera sits within the atmosphere
// 		// Were gonna consider a flat world in our scene and that that Y = 0 is where our earth surface is
// 		// The constants below should match the one in SkyAtmosphereCommon.ush
// 		// Always force to be 5 meters above the ground/sea level (to always see the sky and not be under the virtual planet occluding ray tracing) and lower for small planet radius
// 		const float PlanetRadiusOffset = 0.005f; //Sky units in km
// 
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
// 		Vector4f SkyPlanetTranslatedWorldCenterAndViewHeight = Vector4f(PlanetCenterTranslatedWorld.x,
// 			PlanetCenterTranslatedWorld.y,
// 			PlanetCenterTranslatedWorld.z,
// 			Length(SkyCameraTranslatedWorldOriginTranslatedWorld - PlanetCenterTranslatedWorld));
// 
// 		// Compute the basis vectors for the frame of reference of the sky view LUT. This is a frame of reference tangent to the earth surface at the point the camera is
// 		// Our world is flat and not curved so we consider +Y  the up vector
// 		Mat44 SkyViewLutReferential;
// 		Vector3f ViewForward = Vector3f(renderData.m_CameraData.ViewMatrix[8], renderData.m_CameraData.ViewMatrix[9], renderData.m_CameraData.ViewMatrix[10]);
// 		Vector3f ViewRight = Vector3f(renderData.m_CameraData.ViewMatrix[0], renderData.m_CameraData.ViewMatrix[1], renderData.m_CameraData.ViewMatrix[2]);
// 		Vector3f Up = Vector3f(0,1,0);
// 		Vector3f Forward = ViewForward;		// This can make texel visible when the camera is rotating. Use constant world direction instead?
// 		//FVector3f	Left = normalize(cross(Forward, Up)); 
// 		Vector3f	Left;
// 		Left = vkr::Cross(Forward, Up);
// 		vkr::Normalize(Left);
// 		const float DotMainDir = abs(vkr::Dot(Up, Forward));
// 		if (DotMainDir > 0.999f)
// 		{
// 			// When it becomes hard to generate a referential, generate it procedurally.
// 			// [ Duff et al. 2017, "Building an Orthonormal Basis, Revisited" ]
// 			const float Sign = Up.z >= 0.0f ? 1.0f : -1.0f;
// 			const float a = -1.0f / (Sign + Up.z);
// 			const float b = Up.x * Up.y * a;
// 			Forward = Vector3f(1 + Sign * a * pow(Up.x, 2.0f), Sign * b, -Sign * Up.x);
// 			Left = Vector3f(b, Sign + a * pow(Up.y, 2.0f), -Up.y);
// 
// 			SkyViewLutReferential = vkr::Compose(
// 				Vector4f(Forward.x,Forward.y,Forward.z,0.0f),
// 				Vector4f(Left.x, Left.y, Left.z, 0.0f),
// 				Vector4f(Up.x, Up.y, Up.z, 0.0f),
// 				Vector4f(0.0f,0.0f,0.0f,1.0f)
// 			);
// 			SkyViewLutReferential = SkyViewLutReferential.GetTransposed();
// 		}
// 		else
// 		{
// 			// This is better as it should be more stable with respect to camera forward.
// 			Forward = vkr::Cross(Up, Left);
// 			vkr::Normalize(Forward);
// 			SkyViewLutReferential.SetColumn(0, Forward);
// 			SkyViewLutReferential.SetColumn(1, Left);
// 			SkyViewLutReferential.SetColumn(2, Up);
// 			SkyViewLutReferential = SkyViewLutReferential.GetTransposed();
// 		}
// 
// 		ctx->BindLocalConstantBuffer(sizeof(SkyViewConstantData), &skyViewConstantData, 0);
// 
// 		ctx->DispatchThreads(Vector3u(SKYVIEW_TEXTURE_WIDTH, SKYVIEW_TEXTURE_HEIGHT, 1));
 		
// 
// 		const AtmosphereData& atmosphereData = GetAtmosphereData();
// 
// 		TransmittanceConstantData constantData;
// 		constantData.atmosphere = atmosphereData;
// 		constantData.transmittanceTextureSize = Vector2u(renderTargets.m_TransmittanceLUT.m_Texture->m_TextureDesc.m_Size.x, renderTargets.m_TransmittanceLUT.m_Texture->m_TextureDesc.m_Size.y);
// 		constantData.transmittanceTextureDescriptorIndex = renderTargets.m_TransmittanceLUT.m_TextureViewRW->GetIndex();
// 
// 		ctx->BindLocalConstantBuffer(sizeof(TransmittanceConstantData), &constantData, 0);
// 
// 		ctx->DispatchThreads(Vector3u(constantData.transmittanceTextureSize.x, constantData.transmittanceTextureSize.y, 1));

		//Direct scattering
		
	}
}
