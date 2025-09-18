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

#endif //COMMON_HLSL