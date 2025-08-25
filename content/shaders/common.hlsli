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

#endif //COMMON_HLSL