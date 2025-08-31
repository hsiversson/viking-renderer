#include "skydefinitions.hlsli"

cbuffer Constants : register(b0)
{
    AtmosphereParameters atmosphere;
    uint2 TransmittanceTextureSize;
    uint TransmittanceTextureDescriptorIndex;
}

float GetUnitRangeFromTextureCoord(float u, uint texture_size)
{
    return (u - 0.5 / float(texture_size)) / (1.0f - 1.0f / float(texture_size));
}

float ClampCosine(float mu)
{
    return clamp(mu, float(-1.0), float(1.0));
}

float ClampDistance(float d)
{
    return max(d, 0.0);
}

float SafeSqrt(float a)
{
    return sqrt(max(a, 0.0));
}

float DistanceToTopAtmosphereBoundary(float r, float mu)
{
    float discriminant = r * r * (mu * mu - 1.0) + atmosphere.top_radius * atmosphere.top_radius;
    return ClampDistance(-r * mu + SafeSqrt(discriminant));
}

float GetLayerDensity(in DensityProfileLayer layer,
float altitude) 
{
    float density = layer.exp_term * exp(layer.exp_scale * altitude) +
      layer.linear_term * altitude + layer.constant_term;
    return clamp(density, float(0.0), float(1.0));
}

float GetProfileDensity(in DensityProfile profile,
float altitude) 
{
    return altitude < profile.layers[0].width ?
      GetLayerDensity(profile.layers[0], altitude) :
      GetLayerDensity(profile.layers[1], altitude);
}

float ComputeOpticalLengthToTopAtmosphereBoundary(in DensityProfile profile, float r, float mu) 
{
    // Number of intervals for the numerical integration.
    const int SAMPLE_COUNT = 500;
    // The integration step, i.e. the length of each integration interval.
    float dx = DistanceToTopAtmosphereBoundary(r, mu) / float(SAMPLE_COUNT);
    // Integration loop.
    float result = 0.0;
    for (int i = 0;i <= SAMPLE_COUNT;++i)
    {
        float d_i = float(i) * dx;
        // Distance between the current sample point and the planet center.
        float r_i = sqrt(d_i * d_i + 2.0 * r * mu * d_i + r * r);
        // Number density at the current sample point (divided by the number density
        // at the bottom of the atmosphere, yielding a dimensionless number).
        float y_i = GetProfileDensity(profile, r_i - atmosphere.bottom_radius);
        // Sample weight (from the trapezoidal rule).
        float weight_i = i == 0 || i == SAMPLE_COUNT ? 0.5 : 1.0;
        result += y_i * weight_i * dx;
    }
    return result;
}

float3 ComputeTransmittanceToTopAtmosphereBoundary(float r, float mu) 
{
  //assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
  //assert(mu >= -1.0 && mu <= 1.0);
  return exp(-(
      atmosphere.rayleigh_scattering *
          ComputeOpticalLengthToTopAtmosphereBoundary(
              atmosphere.rayleigh_density, r, mu) +
      atmosphere.mie_extinction *
          ComputeOpticalLengthToTopAtmosphereBoundary(
              atmosphere.mie_density, r, mu) +
      atmosphere.absorption_extinction *
          ComputeOpticalLengthToTopAtmosphereBoundary(
              atmosphere.absorption_density, r, mu)));
}

void GetRMuFromTransmittanceTextureUv(in float2 uv, out float r, out float mu) 
{
    float x_mu = GetUnitRangeFromTextureCoord(uv.x, TransmittanceTextureSize.x);
    float x_r = GetUnitRangeFromTextureCoord(uv.y, TransmittanceTextureSize.y);
    // Distance to top atmosphere boundary for a horizontal ray at ground level.
    float H = sqrt(atmosphere.top_radius * atmosphere.top_radius - atmosphere.bottom_radius * atmosphere.bottom_radius);
    // Distance to the horizon, from which we can compute r:
    float rho = H * x_r;
    r = sqrt(rho * rho + atmosphere.bottom_radius * atmosphere.bottom_radius);
    // Distance to the top atmosphere boundary for the ray (r,mu), and its minimum
    // and maximum values over all mu - obtained for (r,1) and (r,mu_horizon) -
    // from which we can recover mu:
    float d_min = atmosphere.top_radius - r;
    float d_max = rho + H;
    float d = d_min + x_mu * (d_max - d_min);
    mu = d == 0.0 ? 1.0f : (H * H - rho * rho - d * d) / (2.0f * r * d);
    mu = ClampCosine(mu);
}

float3 ComputeTransmittanceToTopAtmosphereBoundaryTexture(in uint2 pixel) 
{
    float r;
    float mu;
    GetRMuFromTransmittanceTextureUv(pixel / TransmittanceTextureSize, r, mu);
    return ComputeTransmittanceToTopAtmosphereBoundary(r, mu);
}


[numthreads(8, 8, 1)]
void MainCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    RWTexture2D<float4> Transmittance = ResourceDescriptorHeap[TransmittanceTextureDescriptorIndex];
    uint2 pixel = dispatchThreadID.xy;
    
    Transmittance[pixel].rgb = ComputeTransmittanceToTopAtmosphereBoundaryTexture(pixel);
}