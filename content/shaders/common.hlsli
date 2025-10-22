#ifndef COMMON_HLSLI
#define COMMON_HLSLI

static const float PI = 3.1415927410125732421875;

static const float FLT_MAX = 3.402823466e+38;
static const float FLT_LOWEST = -FLT_MAX;
static const float FLT_SMALL_VALUE = 1e-3f;
static const float FLT_EPSILON_VALUE = 1e-5f;
static const uint UINT_MAX = 0xffffffff;

bool IsNaN(float aX)
{
    return (asuint(aX) & 0x7fffffff) > 0x7f800000;
}

template<typename T>
T Square(in T v)
{
    return v * v;
}

template<typename T>
T Square2(in T v)
{
    return Square(Square(v));
}

// 4th order polynomial approximation
// 4 VGRP, 16 ALU Full Rate
// 7 * 10^-5 radians precision
// Reference : Handbook of Mathematical Functions (chapter : Elementary Transcendental Functions), M. Abramowitz and I.A. Stegun, Ed.
float acosFast4(float inX)
{
    float x1 = abs(inX);
    float x2 = x1 * x1;
    float x3 = x2 * x1;
    float s;

    s = -0.2121144f * x1 + 1.5707288f;
    s = 0.0742610f * x2 + s;
    s = -0.0187293f * x3 + s;
    s = sqrt(1.0f - x1) * s;

	// acos function mirroring
	// check per platform if compiles to a selector - no branch neeeded
    return inX >= 0.0f ? s : PI - s;
}

float atan2Fast(float y, float x)
{
    float t0 = max(abs(x), abs(y));
    float t1 = min(abs(x), abs(y));
    float t3 = t1 / t0;
    float t4 = t3 * t3;

	// Same polynomial as atanFastPos
    t0 = +0.0872929;
    t0 = t0 * t4 - 0.301895;
    t0 = t0 * t4 + 1.0;
    t3 = t0 * t3;

    t3 = abs(y) > abs(x) ? (0.5 * PI) - t3 : t3;
    t3 = x < 0 ? PI - t3 : t3;
    t3 = y < 0 ? -t3 : t3;

    return t3;
}

float2 atan2Fast(float2 y, float2 x)
{
    return float2(atan2Fast(y.x, x.x), atan2Fast(y.y, x.y));
}

float3 atan2Fast(float3 y, float3 x)
{
    return float3(atan2Fast(y.x, x.x), atan2Fast(y.y, x.y), atan2Fast(y.z, x.z));
}

float4 atan2Fast(float4 y, float4 x)
{
    return float4(atan2Fast(y.x, x.x), atan2Fast(y.y, x.y), atan2Fast(y.z, x.z), atan2Fast(y.w, x.w));
}

#endif //COMMON_HLSL