#include "common.hlsli"

//#define SECOND_ATMOSPHERE_LIGHT_ENABLED
// View data is not available for passes running once per scene (and not once per view).
#if !defined(TRANSMITTANCE_PASS) && !defined(SKYVIEWLUT_PASS)//&& !defined(MULTISCATT_PASS) && !defined(SKYLIGHT_PASS)
//#if !defined(MULTISCATT_PASS) && !defined(SKYLIGHT_PASS)
#define VIEWDATA_AVAILABLE //Defines if we have sceneconstants available
#endif

// Float accuracy offset in Sky unit (km, so this is 1m). Should match the one in FAtmosphereSetup::ComputeViewData
#define PLANET_RADIUS_OFFSET 0.001f
#define M_TO_SKY_UNIT 0.001f; //Converts from meters which is the engine unit to kilometers that is the sky calculations base unit 

static const float FarDepthValue = 0.0f; //We use inverted depth
static const float OutputPreExposure = 1.0f; //TODO: What do we do about this?
static const float ViewPreExposure = 1.0f;
static const float ViewOneOverPreExposure = 1.0f;

//This function converts the float4x4 referential we get from constants that is in viking renderer space into a referential in unreal space with Z up
float3x3 GetUEReferential(float4x4 Source)
{
    float3x3 Result;
    Result[0] = Source[0].zxy;
    Result[1] = Source[1].zxy;
    Result[2] = Source[2].zxy;
    return Result;
}

float2 FromUnitToSubUvs(float2 uv, float4 SizeAndInvSize)
{
    return (uv + 0.5f * SizeAndInvSize.zw) * (SizeAndInvSize.xy / (SizeAndInvSize.xy + 1.0f));
}
float2 FromSubUvsToUnit(float2 uv, float4 SizeAndInvSize)
{
    return (uv - 0.5f * SizeAndInvSize.zw) * (SizeAndInvSize.xy / (SizeAndInvSize.xy - 1.0f));
}

void SkyViewLutParamsToUv(
	in bool IntersectGround, in float ViewZenithCosAngle, in float3 ViewDir, in float ViewHeight, in float BottomRadius, in float4 SkyViewLutSizeAndInvSize,
	out float2 UV)
{
    float Vhorizon = sqrt(ViewHeight * ViewHeight - BottomRadius * BottomRadius);
    float CosBeta = Vhorizon / ViewHeight; // GroundToHorizonCos
    float Beta = acosFast4(CosBeta);
    float ZenithHorizonAngle = PI - Beta;
    float ViewZenithAngle = acosFast4(ViewZenithCosAngle);

    if (!IntersectGround)
    {
        float Coord = ViewZenithAngle / ZenithHorizonAngle;
        Coord = 1.0f - Coord;
        Coord = sqrt(Coord);
        Coord = 1.0f - Coord;
        UV.y = Coord * 0.5f;
    }
    else
    {
        float Coord = (ViewZenithAngle - ZenithHorizonAngle) / Beta;
        Coord = sqrt(Coord);
        UV.y = Coord * 0.5f + 0.5f;
    }

	{
        UV.x = (atan2Fast(-ViewDir.y, -ViewDir.x) + PI) / (2.0f * PI);
    }

	// Constrain uvs to valid sub texel range (avoid zenith derivative issue making LUT usage visible)
    UV = FromUnitToSubUvs(UV, SkyViewLutSizeAndInvSize);
}

void fromTransmittanceLutUVs(
	out float ViewHeight, out float ViewZenithCosAngle,
	in float BottomRadius, in float TopRadius, in float2 UV)
{
    float Xmu = UV.x;
    float Xr = UV.y;

    float H = sqrt(TopRadius * TopRadius - BottomRadius * BottomRadius);
    float Rho = H * Xr;
    ViewHeight = sqrt(Rho * Rho + BottomRadius * BottomRadius);

    float Dmin = TopRadius - ViewHeight;
    float Dmax = Rho + H;
    float D = Dmin + Xmu * (Dmax - Dmin);
    ViewZenithCosAngle = D == 0.0f ? 1.0f : (H * H - Rho * Rho - D * D) / (2.0f * ViewHeight * D);
    ViewZenithCosAngle = clamp(ViewZenithCosAngle, -1.0f, 1.0f);
}

void getTransmittanceLutUvs(
	in float viewHeight, in float viewZenithCosAngle, in float BottomRadius, in float TopRadius,
	out float2 UV)
{
    float H = sqrt(max(0.0f, TopRadius * TopRadius - BottomRadius * BottomRadius));
    float Rho = sqrt(max(0.0f, viewHeight * viewHeight - BottomRadius * BottomRadius));

    float Discriminant = viewHeight * viewHeight * (viewZenithCosAngle * viewZenithCosAngle - 1.0f) + TopRadius * TopRadius;
    float D = max(0.0f, (-viewHeight * viewZenithCosAngle + sqrt(Discriminant))); // Distance to atmosphere boundary

    float Dmin = TopRadius - viewHeight;
    float Dmax = Rho + H;
    float Xmu = (D - Dmin) / (Dmax - Dmin);
    float Xr = Rho / H;

    UV = float2(Xmu, Xr);
	//UV = float2(fromUnitToSubUvs(UV.x, TRANSMITTANCE_TEXTURE_WIDTH), fromUnitToSubUvs(UV.y, TRANSMITTANCE_TEXTURE_HEIGHT)); // No real impact so off
}

void UvToLutTransmittanceParams(out float ViewHeight, out float ViewZenithCosAngle, in float2 UV)
{
	//UV = FromSubUvsToUnit(UV, SkyAtmosphere.TransmittanceLutSizeAndInvSize); // No real impact so off
    fromTransmittanceLutUVs(ViewHeight, ViewZenithCosAngle, Atmosphere.BottomRadiusKm, Atmosphere.TopRadiusKm, UV);
}

void LutTransmittanceParamsToUv(in float ViewHeight, in float ViewZenithCosAngle, out float2 UV)
{
    getTransmittanceLutUvs(ViewHeight, ViewZenithCosAngle, Atmosphere.BottomRadiusKm, Atmosphere.TopRadiusKm, UV);
}

#ifdef VIEWDATA_AVAILABLE
// Used for post process shaders which don't need to resolve the view	
float3 SvPositionToTranslatedWorld(float4 SvPosition)
{
    
   /* float2 uv = (pixel + 0.5f) / float2(width, height);
    uv.y = 1.0 - uv.y;
    float2 ndc = uv * 2.0f - 1.0f;
    float4 clipPos = float4(ndc, depth, 1.0);
    float4 worldPos = mul(SceneConstants.InvViewProjection, clipPos);
    worldPos /= worldPos.w;*/
    
    float4 HomWorldPos = mul(SceneConstants.InvViewProjection, float4(SvPosition.xyz, 1));

    return HomWorldPos.xyz / HomWorldPos.w;
}

float4 GetScreenTranslatedWorldPos(float4 SVPos, float DeviceZ)
{
    DeviceZ = max(0.000000000001, DeviceZ); // TODO: investigate why SvPositionToWorld returns bad values when DeviceZ is far=0 when using inverted z
    return float4(SvPositionToTranslatedWorld(float4(SVPos.xy, DeviceZ, 1.0)), 1.0);
}
#endif

/**
 * Returns near intersection in x, far intersection in y, or both -1 if no intersection.
 * RayDirection does not need to be unit length.
 */
float2 RayIntersectSphere(float3 RayOrigin, float3 RayDirection, float4 Sphere)
{
    float3 LocalPosition = RayOrigin - Sphere.xyz;
    float LocalPositionSqr = dot(LocalPosition, LocalPosition);

    float3 QuadraticCoef;
    QuadraticCoef.x = dot(RayDirection, RayDirection);
    QuadraticCoef.y = 2 * dot(RayDirection, LocalPosition);
    QuadraticCoef.z = LocalPositionSqr - Sphere.w * Sphere.w;

    float Discriminant = QuadraticCoef.y * QuadraticCoef.y - 4 * QuadraticCoef.x * QuadraticCoef.z;

    float2 Intersections = -1;

	// Only continue if the ray intersects the sphere
    //FLATTEN

    if (Discriminant >= 0)
    {
        float SqrtDiscriminant = sqrt(Discriminant);
        Intersections = (-QuadraticCoef.y + float2(-1, 1) * SqrtDiscriminant) / (2 * QuadraticCoef.x);
    }

    return Intersections;
}

// - RayOrigin: ray origin
// - RayDir: normalized ray direction
// - SphereCenter: sphere center
// - SphereRadius: sphere radius
// - Returns distance from RayOrigin to closest intersecion with sphere,
//   or -1.0 if no intersection.
float RaySphereIntersectNearest(float3 RayOrigin, float3 RayDir, float3 SphereCenter, float SphereRadius)
{
    float2 Sol = RayIntersectSphere(RayOrigin, RayDir, float4(SphereCenter, SphereRadius));
    float Sol0 = Sol.x;
    float Sol1 = Sol.y;
    if (Sol0 < 0.0f && Sol1 < 0.0f)
    {
        return -1.0f;
    }
    if (Sol0 < 0.0f)
    {
        return max(0.0f, Sol1);
    }
    else if (Sol1 < 0.0f)
    {
        return max(0.0f, Sol0);
    }
    return max(0.0f, min(Sol0, Sol1));
}

bool MoveToTopAtmosphere(inout float3 WorldPos, in float3 WorldDir, in float AtmosphereTopRadius)
{
    float ViewHeight = length(WorldPos);
    if (ViewHeight > AtmosphereTopRadius)
    {
        float TTop = RaySphereIntersectNearest(WorldPos, WorldDir, float3(0.0f, 0.0f, 0.0f), AtmosphereTopRadius);
        if (TTop >= 0.0f)
        {
            float3 UpVector = WorldPos / ViewHeight;
            float3 UpOffset = UpVector * -PLANET_RADIUS_OFFSET;
            WorldPos = WorldPos + WorldDir * TTop + UpOffset;
        }
        else
        {
			// Ray is not intersecting the atmosphere
            return false;
        }
    }
    return true; // ok to start tracing
}

// Follows PBRT convention http://www.pbr-book.org/3ed-2018/Volume_Scattering/Phase_Functions.html#PhaseHG
float HenyeyGreensteinPhase(float G, float CosTheta)
{
	// Reference implementation (i.e. not schlick approximation). 
	// See http://www.pbr-book.org/3ed-2018/Volume_Scattering/Phase_Functions.html
    float Numer = 1.0f - G * G;
    float Denom = 1.0f + G * G + 2.0f * G * CosTheta;
    return Numer / (4.0f * PI * Denom * sqrt(Denom));
}

float RayleighPhase(float CosTheta)
{
    float Factor = 3.0f / (16.0f * PI);
    return Factor * (1.0f + CosTheta * CosTheta);
}

float3 GetAlbedo(float3 Scattering, float3 Extinction)
{
    return Scattering / max(0.001f, Extinction);
}

struct MediumSampleRGB
{
    float3 Scattering;
    float3 Absorption;
    float3 Extinction;

    float3 ScatteringMie;
    float3 AbsorptionMie;
    float3 ExtinctionMie;

    float3 ScatteringRay;
    float3 AbsorptionRay;
    float3 ExtinctionRay;

    float3 ScatteringOzo;
    float3 AbsorptionOzo;
    float3 ExtinctionOzo;

    float3 Albedo;
};

MediumSampleRGB SampleAtmosphereMediumRGB(in float3 WorldPos)
{
    const float SampleHeight = max(0.0, (length(WorldPos) - Atmosphere.BottomRadiusKm));

    const float DensityMie = exp(Atmosphere.MieDensityExpScale * SampleHeight);

    const float DensityRay = exp(Atmosphere.RayleighDensityExpScale * SampleHeight);

    const float DensityOzo = SampleHeight < Atmosphere.AbsorptionDensity0LayerWidth ?
		saturate(Atmosphere.AbsorptionDensity0LinearTerm * SampleHeight + Atmosphere.AbsorptionDensity0ConstantTerm) : // We use saturate to allow the user to create plateau, and it is free on GCN.
		saturate(Atmosphere.AbsorptionDensity1LinearTerm * SampleHeight + Atmosphere.AbsorptionDensity1ConstantTerm);

    MediumSampleRGB s;

    s.ScatteringMie = DensityMie * Atmosphere.MieScattering.rgb;
    s.AbsorptionMie = DensityMie * Atmosphere.MieAbsorption.rgb;
    s.ExtinctionMie = DensityMie * Atmosphere.MieExtinction.rgb;

    s.ScatteringRay = DensityRay * Atmosphere.RayleighScattering.rgb;
    s.AbsorptionRay = 0.0f;
    s.ExtinctionRay = s.ScatteringRay + s.AbsorptionRay;

    s.ScatteringOzo = 0.0f;
    s.AbsorptionOzo = DensityOzo * Atmosphere.AbsorptionExtinction.rgb;
    s.ExtinctionOzo = s.ScatteringOzo + s.AbsorptionOzo;

    s.Scattering = s.ScatteringMie + s.ScatteringRay + s.ScatteringOzo;
    s.Absorption = s.AbsorptionMie + s.AbsorptionRay + s.AbsorptionOzo;
    s.Extinction = s.ExtinctionMie + s.ExtinctionRay + s.ExtinctionOzo;
    s.Albedo = GetAlbedo(s.Scattering, s.Extinction);

    return s;
}

float3 GetTransmittance(in float LightZenithCosAngle, in float PHeight)
{
    float2 UV;
    LutTransmittanceParamsToUv(PHeight, LightZenithCosAngle, UV);
#ifdef WHITE_TRANSMITTANCE
	float3 TransmittanceToLight = 1.0f;
#else
    float3 TransmittanceToLight = GetTransmittanceLUT().SampleLevel(g_SamplerBilinearClamp, UV, 0).rgb;
#endif
    return TransmittanceToLight;
}

#define DEFAULT_SAMPLE_OFFSET 0.3f
float SkyAtmosphereNoise(float2 UV)
{
	//	return DEFAULT_SAMPLE_OFFSET;
	//	return float(Rand3DPCG32(int3(UV.x, UV.y, S)).x) / 4294967296.0f;
#if defined(VIEWDATA_AVAILABLE) && defined(PER_PIXEL_NOISE)
	return View.RealTimeReflectionCapture ? DEFAULT_SAMPLE_OFFSET : InterleavedGradientNoise(UV.xy, FrameIndex);
#else
    return DEFAULT_SAMPLE_OFFSET;
#endif
}

// In this function, all world position are relative to the planet center (itself expressed within translated world space)
SingleScatteringResult IntegrateSingleScatteredLuminance(
	in float4 SVPos, in float3 WorldPos, in float3 WorldDir,
	in bool Ground, in SamplingSetup Sampling, in float DeviceZ, in bool MieRayPhase,
	in float3 Light0Dir, in float3 Light1Dir, in float3 Light0Illuminance, in float3 Light1Illuminance,
	in float AerialPerspectiveViewDistanceScale,
	in float tMaxMax = 9000000.0f)
{
    SingleScatteringResult Result;
    Result.L = 0;
    Result.LMieOnly = 0;
    Result.LRayOnly = 0;
    Result.OpticalDepth = 0;
    Result.Transmittance = 1.0f;
    Result.TransmittanceMieOnly = 1.0f;
    Result.TransmittanceRayOnly = 1.0f;
    Result.MultiScatAs1 = 0;

    if (dot(WorldPos, WorldPos) <= Atmosphere.BottomRadiusKm * Atmosphere.BottomRadiusKm)
    {
        return Result; // Camera is inside the planet ground
    }

    float2 PixPos = SVPos.xy;

	// Compute next intersection with atmosphere or ground
    float3 PlanetO = float3(0.0f, 0.0f, 0.0f);
    float tMax = 0.0f;
#if 0
	// The bottom code causes the skyview lut to flicker when view from space afar.
	// Remove that code when the else section is proven.
	float tBottom = RaySphereIntersectNearest(WorldPos, WorldDir, PlanetO, Atmosphere.BottomRadiusKm);
	float tTop = RaySphereIntersectNearest(WorldPos, WorldDir, PlanetO, Atmosphere.TopRadiusKm);
	if (tBottom < 0.0f)
	{
		if (tTop < 0.0f)
		{
			tMax = 0.0f; // No intersection with planet nor its atmosphere: stop right away  
			return Result;
		}
		else
		{
			tMax = tTop;
		}
	}
	else
	{
		if (tTop > 0.0f)
		{
			tMax = min(tTop, tBottom);
		}
	}
#else
    float tBottom = 0.0f;
    float2 SolB = RayIntersectSphere(WorldPos, WorldDir, float4(PlanetO, Atmosphere.BottomRadiusKm));
    float2 SolT = RayIntersectSphere(WorldPos, WorldDir, float4(PlanetO, Atmosphere.TopRadiusKm));

    const bool bNoBotIntersection = all(SolB < 0.0f);
    const bool bNoTopIntersection = all(SolT < 0.0f);
    if (bNoTopIntersection)
    {
		// No intersection with planet or its atmosphere.
        tMax = 0.0f;
        return Result;
    }
    else if (bNoBotIntersection)
    {
		// No intersection with planet, so we trace up to the far end of the top atmosphere 
		// (looking up from ground or edges when see from afar in space).
        tMax = max(SolT.x, SolT.y);
    }
    else
    {
		// Interesection with planet and atmospehre: we simply trace up to the planet ground.
		// We know there is at least one intersection thanks to bNoBotIntersection.
		// If one of the solution is invalid=-1, that means we are inside the planet: we stop tracing by setting tBottom=0.
        tBottom = max(0.0f, min(SolB.x, SolB.y));
        tMax = tBottom;
    }
#endif

    float PlanetOnOpaque = 1.0f; // This is used to hide opaque meshes under the planet ground
#ifdef VIEWDATA_AVAILABLE
#ifdef SAMPLE_ATMOSPHERE_ON_CLOUDS
	if (true)
	{
		float tDepth = DeviceZ; // When SAMPLE_ATMOSPHERE_ON_CLOUDS, DeviceZ is world distance in kilometer.
		if (tDepth < tMax)
		{
			tMax = tDepth;
		}
	}
#else // SAMPLE_ATMOSPHERE_ON_CLOUDS
	if (DeviceZ != FarDepthValue)
	{
		const float3 DepthBufferTranslatedWorldPosKm = GetScreenTranslatedWorldPos(SVPos, DeviceZ).xyz * M_TO_SKY_UNIT;
		const float3 TraceStartTranslatedWorldPosKm  = WorldPos + SceneConstants.SkyPlanetTranslatedWorldCenterAndViewHeight.xyz * M_TO_SKY_UNIT; // apply planet offset to go back to world from planet local referencial.
		const float3 TraceStartToSurfaceWorldKm = DepthBufferTranslatedWorldPosKm - TraceStartTranslatedWorldPosKm;
		float tDepth = length(TraceStartToSurfaceWorldKm);
		if (tDepth < tMax)
		{
			tMax = tDepth;
		}
		else
		{
			// Artists did not like that we handle automatic hiding of opaque element behind the planet.
			// Now, pixel under the surface of earht will receive aerial perspective as if they were  on the ground.
			//PlanetOnOpaque = 0.0;
		}

		//if the ray intersects with the atmosphere boundary, make sure we do not apply atmosphere on surfaces are front of it. 
		if (dot(WorldDir, TraceStartToSurfaceWorldKm) < 0.0)
		{
			return Result;
		}
	}
#endif // SAMPLE_ATMOSPHERE_ON_CLOUDS
#endif // VIEWDATA_AVAILABLE
    tMax = min(tMax, tMaxMax);

	// Sample count 
    float SampleCount = Sampling.SampleCountIni;
    float SampleCountFloor = Sampling.SampleCountIni;
    float tMaxFloor = tMax;
    if (Sampling.VariableSampleCount)
    {
        SampleCount = lerp(Sampling.MinSampleCount, Sampling.MaxSampleCount, saturate(tMax * Sampling.DistanceToSampleCountMaxInv));
        SampleCountFloor = floor(SampleCount);
        tMaxFloor = tMax * SampleCountFloor / SampleCount; // rescale tMax to map to the last entire step segment.
    }
    float dt = tMax / SampleCount;

	// Phase functions
    const float uniformPhase = 1.0f / (4.0f * PI);
    const float3 wi = Light0Dir;
    const float3 wo = WorldDir;
    float cosTheta = dot(wi, wo);
    float MiePhaseValueLight0 = HenyeyGreensteinPhase(Atmosphere.MiePhaseG, -cosTheta); // negate cosTheta because due to WorldDir being a "in" direction. 
    float RayleighPhaseValueLight0 = RayleighPhase(cosTheta);
#ifdef SECOND_ATMOSPHERE_LIGHT_ENABLED
	cosTheta = dot(Light1Dir, wo);
	float MiePhaseValueLight1 = HenyeyGreensteinPhase(Atmosphere.MiePhaseG, -cosTheta);	// negate cosTheta because due to WorldDir being a "in" direction. 
	float RayleighPhaseValueLight1 = RayleighPhase(cosTheta);
#endif

	// Ray march the atmosphere to integrate optical depth
    float3 L = 0.0f;
    float3 LMieOnly = 0.0f;
    float3 LRayOnly = 0.0f;
    float3 Throughput = 1.0f;
    float3 ThroughputMieOnly = 1.0f;
    float3 ThroughputRayOnly = 1.0f;
    float3 OpticalDepth = 0.0f;
    float t = 0.0f;
    float tPrev = 0.0f;

    float3 ExposedLight0Illuminance = Light0Illuminance * OutputPreExposure;
#ifdef SECOND_ATMOSPHERE_LIGHT_ENABLED
	float3 ExposedLight1Illuminance = Light1Illuminance * OutputPreExposure;
#endif

//#if SAMPLE_OPAQUE_SHADOW
	// Get the referencial when rendering the SkyView lut being in a special Z-top space
#ifdef SKYVIEWLUT_PASS
	float3x3 LocalReferencial = SkyViewLutReferential;
#endif
//#endif
    
#ifdef PER_PIXEL_NOISE 
    float PixelNoise = SkyAtmosphereNoise(PixPos.xy);
#else
    float PixelNoise = DEFAULT_SAMPLE_OFFSET;
#endif
    
    for (float SampleI = 0.0f; SampleI < SampleCount; SampleI += 1.0f)
    {
		// Compute current ray t and sample point P
        if (Sampling.VariableSampleCount)
        {
			// More expenssive but artefact free
            float t0 = (SampleI) / SampleCountFloor;
            float t1 = (SampleI + 1.0f) / SampleCountFloor;
			// Non linear distribution of samples within the range.
            t0 = t0 * t0;
            t1 = t1 * t1;
			// Make t0 and t1 world space distances.
            t0 = tMaxFloor * t0;
            if (t1 > 1.0f)
            {
                t1 = tMax;
				//t1 = tMaxFloor;	// this reveal depth slices
            }
            else
            {
                t1 = tMaxFloor * t1;
            }
            t = t0 + (t1 - t0) * PixelNoise;
            dt = t1 - t0;
        }
        else
        {
            t = tMax * (SampleI + PixelNoise) / SampleCount;
        }
        float3 P = WorldPos + t * WorldDir;
        float PHeight = length(P);

		// Sample the medium
        MediumSampleRGB Medium = SampleAtmosphereMediumRGB(P);
        const float3 SampleOpticalDepth = Medium.Extinction * dt * AerialPerspectiveViewDistanceScale;
        const float3 SampleTransmittance = exp(-SampleOpticalDepth);
        OpticalDepth += SampleOpticalDepth;

		// Transmittance Ray only and Mie only set half of ozone in rayleigh and half of ozone in mie parts.
		// This is not great but I do not have any better solution. 
		// Also most of the time Ozone is high in the atmosphere so it should be fine this way.
        ThroughputMieOnly *= exp(-(Medium.ExtinctionMie + Medium.ExtinctionOzo) * dt * AerialPerspectiveViewDistanceScale);
        ThroughputRayOnly *= exp(-(Medium.ExtinctionRay + Medium.ExtinctionOzo) * dt * AerialPerspectiveViewDistanceScale);

		// Phase and transmittance for light 0
        const float3 UpVector = P / PHeight;
        float Light0ZenithCosAngle = dot(Light0Dir, UpVector);
        float3 TransmittanceToLight0 = GetTransmittance(Light0ZenithCosAngle, PHeight);
        float3 PhaseTimesScattering0;
        float3 PhaseTimesScattering0MieOnly;
        float3 PhaseTimesScattering0RayOnly;
        if (MieRayPhase)
        {
            PhaseTimesScattering0MieOnly = Medium.ScatteringMie * MiePhaseValueLight0;
            PhaseTimesScattering0RayOnly = Medium.ScatteringRay * RayleighPhaseValueLight0;
            PhaseTimesScattering0 = PhaseTimesScattering0MieOnly + PhaseTimesScattering0RayOnly;
        }
        else
        {
            PhaseTimesScattering0MieOnly = Medium.ScatteringMie * uniformPhase;
            PhaseTimesScattering0RayOnly = Medium.ScatteringRay * uniformPhase;
            PhaseTimesScattering0 = Medium.Scattering * uniformPhase;
        }
#ifdef SECOND_ATMOSPHERE_LIGHT_ENABLED
		// Phase and transmittance for light 1
		float Light1ZenithCosAngle = dot(Light1Dir, UpVector);
		float3 TransmittanceToLight1 = GetTransmittance(Light1ZenithCosAngle, PHeight);
		float3 PhaseTimesScattering1;
		float3 PhaseTimesScattering1MieOnly;
		float3 PhaseTimesScattering1RayOnly;
		if (MieRayPhase)
		{
			PhaseTimesScattering1MieOnly= Medium.ScatteringMie * MiePhaseValueLight1;
			PhaseTimesScattering1RayOnly= Medium.ScatteringRay * RayleighPhaseValueLight1;
			PhaseTimesScattering1		= PhaseTimesScattering1MieOnly + PhaseTimesScattering1RayOnly;
		}
		else
		{
			PhaseTimesScattering1MieOnly= Medium.ScatteringMie * uniformPhase;
			PhaseTimesScattering1RayOnly= Medium.ScatteringRay * uniformPhase;
			PhaseTimesScattering1		= Medium.Scattering * uniformPhase;
		}
#endif // SECOND_ATMOSPHERE_LIGHT_ENABLED

		// Multiple scattering approximation
        float3 MultiScatteredLuminance0 = 0.0f;
#ifdef MULTISCATTERING_APPROX_SAMPLING_ENABLED
		MultiScatteredLuminance0 = GetMultipleScattering(P, Light0ZenithCosAngle);
#endif
#ifdef SECOND_ATMOSPHERE_LIGHT_ENABLED
		float3 MultiScatteredLuminance1 = 0.0f;
#ifdef MULTISCATTERING_APPROX_SAMPLING_ENABLED
		MultiScatteredLuminance1 = GetMultipleScattering(P, Light1ZenithCosAngle);
#endif
#endif

		// Planet shadow
        float tPlanet0 = RaySphereIntersectNearest(P, Light0Dir, PlanetO + PLANET_RADIUS_OFFSET * UpVector, Atmosphere.BottomRadiusKm);
        float PlanetShadow0 = tPlanet0 >= 0.0f ? 0.0f : 1.0f;
        float3 ShadowP0 = P;
        bool bUnused = false;
#ifdef SKYVIEWLUT_PASS
        float3 SkyCameraTranslatedWorldOrigin = float3(0.0f,0.0f,0.0f); // TODO: need to uderstand better the mess of references they work with in unreal. Potential bug here
        float3 TranslatedCameraPlanetPos = (SkyCameraTranslatedWorldOrigin - SkyPlanetTranslatedWorldCenterAndViewHeight.xyz) * M_TO_SKY_UNIT;
		ShadowP0 = TranslatedCameraPlanetPos + t * mul(LocalReferencial, WorldDir); // Inverse of the local SkyViewLUT referencial transform
#endif
#ifdef SAMPLE_OPAQUE_SHADOW
		{
			float3 ShadowSampleWorldPosition0 = ShadowP0 * SKY_UNIT_TO_CM + SceneConstants.SkyPlanetTranslatedWorldCenterAndViewHeight.xyz;
			PlanetShadow0 *= ComputeLight0VolumeShadowing(ShadowSampleWorldPosition0 /* - DFHackToFloat(PrimaryView.PreViewTranslation)*/, false, false, bUnused);

#ifdef VIRTUAL_SHADOW_MAP
			if (VirtualShadowMapId0 != INDEX_NONE)
			{
				FVirtualShadowMapSampleResult VirtualShadowMapSample = SampleVirtualShadowMapDirectional(VirtualShadowMapId0, ShadowSampleWorldPosition0);
				PlanetShadow0 *= VirtualShadowMapSample.ShadowFactor;
			}
#endif // VIRTUALSHADOW_MAP
		}
#endif
#ifdef SAMPLE_CLOUD_SKYAO
		float OutOpticalDepth = 0.0f;
		MultiScatteredLuminance0 *= GetCloudVolumetricShadow(ShadowP0 * SKY_UNIT_TO_CM + SceneConstants.SkyPlanetTranslatedWorldCenterAndViewHeight.xyz, VolumetricCloudCommonParameters.CloudSkyAOTranslatedWorldToLightClipMatrix,
			VolumetricCloudCommonParameters.CloudSkyAOFarDepthKm, VolumetricCloudSkyAOTexture, VolumetricCloudSkyAOTextureSampler, OutOpticalDepth);
#endif
#ifdef SAMPLE_CLOUD_SHADOW
		float OutOpticalDepth2 = 0.0f;
		PlanetShadow0 *= saturate(lerp(1.0f, GetCloudVolumetricShadow(ShadowP0 * SKY_UNIT_TO_CM + SceneConstants.SkyPlanetTranslatedWorldCenterAndViewHeight.xyz, VolumetricCloudCommonParameters.CloudShadowmapTranslatedWorldToLightClipMatrix[0],
			VolumetricCloudCommonParameters.CloudShadowmapFarDepthKm[0].x, VolumetricCloudShadowMapTexture0, VolumetricCloudShadowMapTexture0Sampler, OutOpticalDepth2), VolumetricCloudShadowStrength0));
#endif
		// MultiScatteredLuminance is already pre-exposed, atmospheric light contribution needs to be pre exposed
		// Multi-scattering is also not affected by PlanetShadow or TransmittanceToLight because it contains diffuse light after single scattering.
        float3 S = ExposedLight0Illuminance * (PlanetShadow0 * TransmittanceToLight0 * PhaseTimesScattering0 + MultiScatteredLuminance0 * Medium.Scattering);
        float3 SMieOnly = ExposedLight0Illuminance * (PlanetShadow0 * TransmittanceToLight0 * PhaseTimesScattering0MieOnly + MultiScatteredLuminance0 * Medium.ScatteringMie);
        float3 SRayOnly = ExposedLight0Illuminance * (PlanetShadow0 * TransmittanceToLight0 * PhaseTimesScattering0RayOnly + MultiScatteredLuminance0 * Medium.ScatteringRay);

#ifdef SECOND_ATMOSPHERE_LIGHT_ENABLED
		float tPlanet1 = RaySphereIntersectNearest(P, Light1Dir, PlanetO + PLANET_RADIUS_OFFSET * UpVector, Atmosphere.BottomRadiusKm);
		float PlanetShadow1 = tPlanet1 >= 0.0f ? 0.0f : 1.0f;
		float3 ShadowP1 = P;
#ifdef SAMPLE_OPAQUE_SHADOW
#ifdef SKYVIEWLUT_PASS
		ShadowP1 = GetTranslatedCameraPlanetPos() + t * mul(LocalReferencial, WorldDir); // Inverse of the local SkyViewLUT referencial transform
#endif
		{
			float3 ShadowSampleWorldPosition1 = ShadowP1 * SKY_UNIT_TO_CM + SceneConstants.SkyPlanetTranslatedWorldCenterAndViewHeight.xyz;
			PlanetShadow1 *= ComputeLight1VolumeShadowing(ShadowSampleWorldPosition1/* - DFHackToFloat(PrimaryView.PreViewTranslation)*/, false, false, bUnused);
#ifdef VIRTUAL_SHADOW_MAP
			if (VirtualShadowMapId1 != INDEX_NONE)
			{
				FVirtualShadowMapSampleResult VirtualShadowMapSample = SampleVirtualShadowMapDirectional(VirtualShadowMapId1, ShadowSampleWorldPosition1);
				PlanetShadow1 *= VirtualShadowMapSample.ShadowFactor;
			}
#endif // VIRTUALSHADOW_MAP
		}
#endif // SAMPLE_OPAQUE_SHADOW
#ifdef SAMPLE_CLOUD_SHADOW
		float OutOpticalDepth3 = 0.0f;
		PlanetShadow1 *= saturate(lerp(1.0f, GetCloudVolumetricShadow(ShadowP1 * SKY_UNIT_TO_CM + SceneConstants.SkyPlanetTranslatedWorldCenterAndViewHeight.xyz, VolumetricCloudCommonParameters.CloudShadowmapTranslatedWorldToLightClipMatrix[1],
			VolumetricCloudCommonParameters.CloudShadowmapFarDepthKm[1].x, VolumetricCloudShadowMapTexture1, VolumetricCloudShadowMapTexture1Sampler, OutOpticalDepth3), VolumetricCloudShadowStrength1));
#endif
		//  Multi-scattering can work for the second light but it is disabled for the sake of performance.
		S		 += ExposedLight1Illuminance * (PlanetShadow1 * TransmittanceToLight1 * PhaseTimesScattering1		 + MultiScatteredLuminance1 * Medium.Scattering);
		SMieOnly += ExposedLight1Illuminance * (PlanetShadow1 * TransmittanceToLight1 * PhaseTimesScattering1MieOnly + MultiScatteredLuminance1 * Medium.ScatteringMie);
		SRayOnly += ExposedLight1Illuminance * (PlanetShadow1 * TransmittanceToLight1 * PhaseTimesScattering1RayOnly + MultiScatteredLuminance1 * Medium.ScatteringRay);
#endif

		// When using the power serie to accumulate all sattering order, serie r must be <1 for a serie to converge. 
		// Under extreme coefficient, MultiScatAs1 can grow larger and thus results in broken visuals. 
		// The way to fix that is to use a proper analytical integration as porposed in slide 28 of http://www.frostbite.com/2015/08/physically-based-unified-volumetric-rendering-in-frostbite/ 
		// However, it is possible to disable as it can also work using simple power serie sum unroll up to 5th order. The rest of the orders has a really low contribution. 
#define MULTI_SCATTERING_POWER_SERIE 0 
        const float3 SafeMediumExtinction = max(Medium.Extinction, 1.e-9);
#if MULTI_SCATTERING_POWER_SERIE==0 
		// 1 is the integration of luminance over the 4pi of a sphere, and assuming an isotropic phase function of 1.0/(4*PI) 
        Result.MultiScatAs1 += Throughput * Medium.Scattering * 1.0f * dt;
#else 
		float3 MS = Medium.Scattering * 1;
		float3 MSint = (MS - MS * SampleTransmittance) / SafeMediumExtinction;
		Result.MultiScatAs1 += Throughput * MSint;
#endif 

#if 0
		L			+= Throughput * S * dt;
		LMieOnly	+= Throughput * SMieOnly * dt;
		LRayOnly	+= Throughput * SRayOnly * dt;
		Throughput	*= SampleTransmittance;
#else
		// See slide 28 at http://www.frostbite.com/2015/08/physically-based-unified-volumetric-rendering-in-frostbite/ 
        float3 Sint = (S - S * SampleTransmittance) / SafeMediumExtinction; // integrate along the current step segment 
        float3 SintMieOnly = (SMieOnly - SMieOnly * SampleTransmittance) / SafeMediumExtinction;
        float3 SintRayOnly = (SRayOnly - SRayOnly * SampleTransmittance) / SafeMediumExtinction;
        L += Throughput * Sint; // accumulate and also take into account the transmittance from previous steps
        LMieOnly += Throughput * SintMieOnly;
        LRayOnly += Throughput * SintRayOnly;
        Throughput *= SampleTransmittance;
#endif

        tPrev = t;
    }

    if (Ground && tMax == tBottom)
    {
		// Account for bounced light off the planet
        float3 P = WorldPos + tBottom * WorldDir;
        float PHeight = length(P);

        const float3 UpVector = P / PHeight;
        float Light0ZenithCosAngle = dot(Light0Dir, UpVector);
        float3 TransmittanceToLight0 = GetTransmittance(Light0ZenithCosAngle, PHeight);

        const float NdotL0 = saturate(dot(UpVector, Light0Dir));
        L += Light0Illuminance * TransmittanceToLight0 * Throughput * NdotL0 * Atmosphere.GroundAlbedo.rgb / PI;
#ifdef SECOND_ATMOSPHERE_LIGHT_ENABLED
		{
			const float NdotL1 = saturate(dot(UpVector, Light1Dir));
			float Light1ZenithCosAngle = dot(UpVector, Light1Dir);
			float3 TransmittanceToLight1 = GetTransmittance(Light1ZenithCosAngle, PHeight);
			L += Light1Illuminance * TransmittanceToLight1 * Throughput * NdotL1 * Atmosphere.GroundAlbedo.rgb / PI;
		}
#endif
    }

    Result.L = L;
    Result.LMieOnly = LMieOnly;
    Result.LRayOnly = LRayOnly;
    Result.OpticalDepth = OpticalDepth;
    Result.Transmittance = Throughput * PlanetOnOpaque;
    Result.TransmittanceMieOnly = ThroughputMieOnly * PlanetOnOpaque;
    Result.TransmittanceRayOnly = ThroughputRayOnly * PlanetOnOpaque;

    return Result;
}

#ifdef SOURCE_DISK_ENABLED

float3 GetAtmosphereTransmittance(
	float3 PlanetCenterToWorldPos, float3 WorldDir, float BottomRadius, float TopRadius,
	Texture2D<float4> TransmittanceLutTexture, SamplerState TransmittanceLutTextureSampler)
{
	// For each view height entry, transmittance is only stored from zenith to horizon. Earth shadow is not accounted for.
	// It does not contain earth shadow in order to avoid texel linear interpolation artefact when LUT is low resolution.
	// As such, at the most shadowed point of the LUT when close to horizon, pure black with earth shadow is never hit.
	// That is why we analytically compute the virtual planet shadow here.
    const float2 Sol = RayIntersectSphere(PlanetCenterToWorldPos, WorldDir, float4(float3(0.0f, 0.0f, 0.0f), BottomRadius));
    if (Sol.x > 0.0f || Sol.y > 0.0f)
    {
        return 0.0f;
    }

    const float PHeight = length(PlanetCenterToWorldPos);
    const float3 UpVector = PlanetCenterToWorldPos / PHeight;
    const float LightZenithCosAngle = dot(WorldDir, UpVector);
    float2 TransmittanceLutUv;
    getTransmittanceLutUvs(PHeight, LightZenithCosAngle, BottomRadius, TopRadius, TransmittanceLutUv);
    const float3 TransmittanceToLight = GetTransmittanceLUT().SampleLevel(g_SamplerBilinearClamp, TransmittanceLutUv, 0.0f).rgb;
    return TransmittanceToLight;
}

float3 GetLightDiskLuminance(
	float3 PlanetCenterToWorldPos, float3 WorldDir, float BottomRadius, float TopRadius,
	Texture2D<float4> TransmittanceLutTexture, SamplerState TransmittanceLutTextureSampler,
	float3 AtmosphereLightDirection, float AtmosphereLightDiscCosHalfApexAngle, float3 AtmosphereLightDiscLuminance)
{
	const float ViewDotLight = dot(WorldDir, AtmosphereLightDirection);
	const float CosHalfApex = AtmosphereLightDiscCosHalfApexAngle;
	if (ViewDotLight > CosHalfApex)
	{
		const float3 TransmittanceToLight = GetAtmosphereTransmittance(
			PlanetCenterToWorldPos, WorldDir, BottomRadius, TopRadius, TransmittanceLutTexture, TransmittanceLutTextureSampler);

		// Soften out the sun disk to avoid bloom flickering at edge. The soften is applied on the outer part of the disk.
		const float SoftEdge = saturate(2.0f * (ViewDotLight - CosHalfApex) / (1.0f - CosHalfApex));

		return TransmittanceToLight * AtmosphereLightDiscLuminance * SoftEdge;
	}
	return 0.0f;
}
#endif