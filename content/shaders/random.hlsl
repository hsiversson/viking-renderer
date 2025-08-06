#ifndef RANDOM_HLSL
#define RANDOM_HLSL

float InterleavedGradientNoise(float2 uv, uint frameIndex)
{
    uv += frameIndex * (float2(47.0f, 17.0f) * 0.695f);
    const float3 magic = float3(0.06711056f, 0.00583715f, 52.9829189f);
    return frac(magic.z * frac(dot(uv, magic.xy)));
}

uint TeaHash(uint x, uint y, uint seed)
{
    uint v0 = x;
    uint v1 = y;
    uint s0 = seed | 1; // some seed
    for (uint i = 0; i < 16; ++i)
    {
        s0 += 0x9e3779b9;
        v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
        v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
    }
    return v0;
}

uint PcgHash(inout uint state)
{
    uint s = state * 747796405u + 2891336453u;
    uint w = ((s >> ((s >> 28u) + 4u)) ^ s) * 277803737u;
    state = (w >> 22u) ^ w;
    return state;
}

uint GenerateRandomSeed(uint x, uint y, uint seed)
{
    return TeaHash(x, y, seed);
}

uint RandomUint(inout uint state)
{
    return PcgHash(state);
}

float RandomFloat01(inout uint state)
{
    return saturate(PcgHash(state) / float(uint(0xffffffff)));
}

#endif //RANDOM_HLSL