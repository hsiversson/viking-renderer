#include "common.hlsli"
#include "random.hlsli"

// Float accuracy offset in Sky unit (km, so this is 1m). Should match the one in FAtmosphereSetup::ComputeViewData
#define PLANET_RADIUS_OFFSET 0.001f
#define M_TO_SKY_UNIT 0.001f; //Converts from meters which is the engine unit to kilometers that is the sky calculations base unit 

static const float FarDepthValue = 0.0f; //We use inverted depth
static const float OutputPreExposure = 1.0f; //TODO: What do we do about this?

struct AtmosphereParameters
{
    float MultiScatteringFactor;
    // The distance between the planet center and the bottom of the atmosphere.
    float BottomRadiusKm;
    // The distance between the ground and the top of the atmosphere.
    float TopRadiusKm;
    float RayleighDensityExpScale;
    float3 RayleighScattering;
    float3 MieScattering;
    float MieDensityExpScale;
    float3 MieExtinction;
    float MiePhaseG;
    float3 MieAbsorption;
    float AbsorptionDensity0LayerWidth;
    float AbsorptionDensity0ConstantTerm;
    float AbsorptionDensity0LinearTerm;
    float AbsorptionDensity1ConstantTerm;
    float AbsorptionDensity1LinearTerm;
    float3 AbsorptionExtinction;
    // The average albedo of the ground.
    float3 GroundAlbedo;
};

struct SamplingSetup
{
    bool VariableSampleCount;
    float SampleCountIni; // Used when VariableSampleCount is false
    float MinSampleCount;
    float MaxSampleCount;
    float DistanceToSampleCountMaxInv;
};

struct SingleScatteringResult
{
    float3 L; // Scattered light (luminance)
    float3 LMieOnly; // L but Mie scattering only
    float3 LRayOnly; // L but Rayleigh scattering only
    float3 OpticalDepth; // Optical depth (1/m)
    float3 Transmittance; // Transmittance in [0,1] (unitless)
    float3 TransmittanceMieOnly; // Transmittance in [0,1] (unitless) but Mie scattering only
    float3 TransmittanceRayOnly; // Transmittance in [0,1] (unitless) but Rayleigh scattering only
    float3 MultiScatAs1;
};