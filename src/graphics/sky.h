
#pragma once

namespace vkr::Render
{
	class PipelineState;
	class Shader;
}

namespace vkr::Graphics
{
	class View;

	struct TentDistribution
	{
		float TipAltitude = 0.0f;
		float TipValue = 0.0f;
		float Width = 1.0f;
	};

	struct AtmosphereParams
	{
		AtmosphereParams();
		/** The radius in kilometers from the center of the planet to the ground level. */
		float BottomRadius;
		/** The ground albedo in sRGB space that will tint the atmosphere when the sun light will bounce on it. Only taken into account when MultiScattering>0.0. */
		Vector3f GroundAlbedo;
		/** The height of the atmosphere layer above the ground in kilometers. */
		float AtmosphereHeight;
		/** Factor applied to multiple scattering only (after the sun light has bounced around in the atmosphere at least once).
		 * Multiple scattering is evaluated using a dual scattering approach.
		 * A value of 2 is recommended to better represent default atmosphere when r.SkyAtmosphere.MultiScatteringLUT.HighQuality=0.
		 */
		float MultiScatteringFactor;
		/**
		 * Scale the atmosphere tracing sample count. Quality level scalability
		 * The sample count is still clamped according to scalability setting to 'r.SkyAtmosphere.SampleCountMax' when 'r.SkyAtmosphere.FastSkyLUT' is 0.
		 * The sample count is still clamped according to scalability setting to 'r.SkyAtmosphere.FastSkyLUT.SampleCountMax' when 'r.SkyAtmosphere.FastSkyLUT' is 1.
		 * The sample count is still clamped for aerial perspective according to  'r.SkyAtmosphere.AerialPerspectiveLUT.SampleCountMaxPerSlice'.
		 */
		float TraceSampleCountScale;
		/** Rayleigh scattering coefficient scale.*/
		float RayleighScatteringScale;
		/** The Rayleigh scattering coefficients resulting from molecules in the air at an altitude of 0 kilometer. */
		Vector3f RayleighScattering;
		/** The altitude in kilometer at which Rayleigh scattering effect is reduced to 40%.*/
		float RayleighExponentialDistribution;
		/** Mie scattering coefficient scale.*/
		float MieScatteringScale;
		/** The Mie scattering coefficients resulting from particles in the air at an altitude of 0 kilometer. As it becomes higher, light will be scattered more. */
		Vector3f MieScattering;
		/** Mie absorption coefficient scale.*/
		float MieAbsorptionScale;
		/** The Mie absorption coefficients resulting from particles in the air at an altitude of 0 kilometer. As it becomes higher, light will be absorbed more. */
		Vector3f MieAbsorption;
		/** A value of 0 mean light is uniformly scattered. A value closer to 1 means lights will scatter more forward, resulting in halos around light sources. */
		float MieAnisotropy;
		/** The altitude in kilometer at which Mie effects are reduced to 40%.*/
		float MieExponentialDistribution;
		/** Absorption coefficients for another atmosphere layer. Density increase from 0 to 1 between 10 to 25km and decreases from 1 to 0 between 25 to 40km. This approximates ozone molecules distribution in the Earth atmosphere. */
		float OtherAbsorptionScale;
		/** Absorption coefficients for another atmosphere layer. Density increase from 0 to 1 between 10 to 25km and decreases from 1 to 0 between 25 to 40km. The default values represents ozone molecules absorption in the Earth atmosphere. */
		Vector3f OtherAbsorption;
		/** Represents the altitude based tent distribution of absorption particles in the atmosphere. */
		TentDistribution OtherTentDistribution;
		/** Scales the luminance of pixels representing the sky. This will impact the captured sky light. */
		Vector3f SkyLuminanceFactor;
		/** Makes the aerial perspective look thicker by scaling distances from view to surfaces (opaque and translucent). */
		float AerialPespectiveViewDistanceScale;
		/** Scale the sky and atmosphere lights contribution to the height fog when SupportSkyAtmosphereAffectsHeightFog project setting is true.*/
		float HeightFogContribution;
		/** The minimum elevation angle in degree that should be used to evaluate the sun transmittance to the ground. Useful to maintain a visible sun light and shadow on meshes even when the sun has started going below the horizon. This does not affect the aerial perspective.*/
		float TransmittanceMinLightElevationAngle;
		/** The distance (kilometers) at which we start evaluating the aerial perspective. Having the aerial perspective starts away from the camera can help with performance: pixels not affected by the aerial perspective will have their computation skipped using early depth test.*/
		float AerialPerspectiveStartDepth;
	};

	class Sky
	{
	public:
		Sky() = default;
		~Sky() = default;

		bool Init();

		void PrepareView(View* view);

	private:
		AtmosphereParams m_AtmosphereParams;
	};

	class SkyRenderer
	{
	public:
		SkyRenderer() = default;
		~SkyRenderer() = default;

		bool Init();

		void ComputeLuts(View* view);

	private:
		Ref<Render::Shader> m_SkyTransmittanceLUTComputeShader;
		Ref<Render::Shader> m_SkyMultiScatterLUTComputeShader;
		Ref<Render::Shader> m_SkyViewLUTComputeShader;
		Ref<Render::PipelineState> m_SkyTransmittanceLUTPSO;
		Ref<Render::PipelineState> m_SkyMultiScatterLUTPSO;
		Ref<Render::PipelineState> m_SkyViewLUTPSO;
	};
}