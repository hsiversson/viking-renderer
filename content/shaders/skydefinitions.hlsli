#include "common.hlsli"
#include "random.hlsli"

struct AtmosphereParameters
{
    float MultiScatteringFactor;
    // The distance between the planet center and the bottom of the atmosphere.
    float BottomRadiusKm;
    // The distance between the ground and the top of the atmosphere.
    float TopRadiusKm;
    float RayleighDensityExpScale;
    float3 RayleighScattering;
    uint pad0;
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
    uint pad1;
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