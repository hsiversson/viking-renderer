#ifndef RANDOM_HLSLI
#define RANDOM_HLSLI

float InterleavedGradientNoise(float2 uv, uint frameIndex)
{
    uv += frameIndex * (float2(47.0f, 17.0f) * 0.695f);
    const float3 magic = float3(0.06711056f, 0.00583715f, 52.9829189f);
    return frac(magic.z * frac(dot(uv, magic.xy)));
}

uint xxhash32(uint p)
{
    const uint PRIME32_2 = 2246822519U;
    const uint PRIME32_3 = 3266489917U;
    const uint PRIME32_4 = 668265263U;
    const uint PRIME32_5 = 374761393U;
    uint h32 = p + PRIME32_5;
    h32 = PRIME32_4 * ((h32 << 17) | (h32 >> (32 - 17)));
    h32 = PRIME32_2 * (h32 ^ (h32 >> 15));
    h32 = PRIME32_3 * (h32 ^ (h32 >> 13));
    return h32 ^ (h32 >> 16);
}

uint xxhash32(uint x, uint y, uint seed)
{
    const uint PRIME32_2 = 2246822519U;
    const uint PRIME32_3 = 3266489917U;
    const uint PRIME32_4 = 668265263U;
    const uint PRIME32_5 = 374761393U;
    uint h32 = x + PRIME32_5 + seed * PRIME32_3;
    h32 = PRIME32_4 * ((h32 << 17) | (h32 >> (32 - 17)));
    h32 += y * PRIME32_3;
    h32 = PRIME32_4 * ((h32 << 17) | (h32 >> (32 - 17)));
    h32 = PRIME32_2 * (h32 ^ (h32 >> 15));
    h32 = PRIME32_3 * (h32 ^ (h32 >> 13));
    return h32 ^ (h32 >> 16);
}

uint GenerateRandomSeed(uint x, uint y, uint seed)
{
    return xxhash32(x, y, seed);
}

uint RandomUint(inout uint state)
{
    return xxhash32(state);
}

float RandomFloat01(inout uint state)
{
    return saturate(xxhash32(state) / float(uint(0xffffffff)));
}

#endif //RANDOM_HLSL