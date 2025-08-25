#ifndef COLOR_COMMON_HLSLI
#define COLOR_COMMON_HLSLI

#include "common.hlsli"

static const uint DISPLAY_ENCODING_TYPE_SRGB = 0;
static const uint DISPLAY_ENCODING_TYPE_ST2048 = 1;
static const uint DISPLAY_ENCODING_TYPE_HLG = 2;

// sRGB transfer functions
float EncodeSRGB(in float x)
{
    x = saturate(x);
    return (x <= 0.0031308f) ? 12.92f * x : 1.055f * pow(x, 1.0f / 2.4f) - 0.055f;
}

float DecodeSRGB(in float x)
{
    x = saturate(x);
    return (x <= 0.04045f) ? x / 12.92f : pow((x + 0.055f) / 1.055f, 2.4f);
}

float3 EncodeSRGB(in float3 linearRgb)
{
    return float3(EncodeSRGB(linearRgb.r), EncodeSRGB(linearRgb.g), EncodeSRGB(linearRgb.b));
}

float3 DecodeSRGB(in float3 encodedRgb)
{
    return float3(DecodeSRGB(encodedRgb.r), DecodeSRGB(encodedRgb.g), DecodeSRGB(encodedRgb.b));
}

// SMPTE ST 2084 (PQ) transfer functions
// Luminance (nits) range: [0, 10000]
static const float St2048_m1 = (2610.0f / 16384.0f);
static const float St2048_m2 = (2523.0f / 32.0f);
static const float St2048_c1 = (3424.0f / 4096.0f);
static const float St2048_c2 = (2413.0f / 128.0f);
static const float St2048_c3 = (2392.0f / 128.0f);
static const float St2048_ep = 10000.0f; // Encoding peak brightness

float EncodeSt2048(in float L)
{
    L = max(0.0f, L);
    float Lm1 = pow(L, St2048_m1);
    float num = St2048_c1 + St2048_c2 * Lm1;
    float den = 1.0f + St2048_c3 * Lm1;
    return pow(num / max(den, 1e-9f), St2048_m2);
}

float DecodeSt2048(in float N)
{
    N = saturate(N);
    float Np = pow(N, 1.0f / St2048_m2);
    float num = max(Np - St2048_c1, 0.0f);
    float den = St2048_c2 - St2048_c3 * Np;
    return pow(num / max(den, 1e-9f), 1.0f / St2048_m1); // nits, up to 10000
}

float3 EncodeSt2048(in float3 linearRgb)
{
    return float3(EncodeSt2048(linearRgb.r), EncodeSt2048(linearRgb.g), EncodeSt2048(linearRgb.b));
}

float3 DecodeSt2048(in float3 encodedRgb)
{
    return float3(DecodeSt2048(encodedRgb.r), DecodeSt2048(encodedRgb.g), DecodeSt2048(encodedRgb.b));
}

// HLG (ITU-R BT.2100) transfer functions
static const float HLG_a = 0.17883277;
static const float HLG_b = 1.0 - 4.0 * HLG_a;
static const float HLG_c = 0.5 - HLG_a * log(4.0 * HLG_a);

float EncodeHLG(in float L)
{
    L = max(L, 0.0f);
    return (L <= 1.0f / 12.0) ? sqrt(3.0f * L) : HLG_a * log(12.0f * L - HLG_b) + HLG_c;
}

float DecodeHLG(in float V)
{
    V = saturate(V);
    return (V <= 0.5f) ? (V * V) / 3.0f : (exp((V - HLG_c) / HLG_a) + HLG_b) / 12.0f;
}

float3 EncodeHLG(in float3 linearRgb)
{
    return float3(EncodeHLG(linearRgb.r), EncodeHLG(linearRgb.g), EncodeHLG(linearRgb.b));
}

float3 DecodeHLG(in float3 encodedRgb)
{
    return float3(DecodeHLG(encodedRgb.r), DecodeHLG(encodedRgb.g), DecodeHLG(encodedRgb.b));
}

#endif // COLOR_COMMON_HLSL