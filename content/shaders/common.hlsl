#ifndef COMMON_HLSL
#define COMMON_HLSL

static const float PI = 3.1415927410125732421875;

static const float FLT_MAX = 3.402823466e+38;
static const float FLT_LOWEST = -FLT_MAX;
static const float FLT_SMALL_VALUE = 0.0001f;
static const float FLT_EPSILON_VALUE = 0.000001f;
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

float InterleavedGradientNoise(uint2 pixel, uint frameIndex)
{
    // Offset pixel coords by frame index to animate the noise
    pixel += frameIndex * uint2(37, 59); // use primes to avoid repetition
    
    // Converts uint to float in [0,1)
    float x = float(pixel.x);
    float y = float(pixel.y);

    // Magic constants to scramble pattern
    return frac(52.9829189f * frac(0.06711056f * x + 0.00583715f * y));
}

#endif //COMMON_HLSL