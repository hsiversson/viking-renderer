#ifndef COLOR_COMMON_HLSLI
#define COLOR_COMMON_HLSLI

#include "common.hlsli"

static const float2 WHITE_POINT_D60 = float2(0.32168f, 0.33767f);
static const float2 WHITE_POINT_D65 = float2(0.31271f, 0.32902f);
static const float2 WHITE_POINT_E = float2(1.0f/3.0f, 1.0f/3.0f);

struct Chromaticity
{
    float2 m_Red;
    float2 m_Green;
    float2 m_Blue;
    float2 m_White;
};
static const Chromaticity CHROMATICITY_XYZ = { float2(1.0f, 0.0f), float2(0.0f, 1.0f), float2(0.0f, 0.0f), WHITE_POINT_E };
static const Chromaticity CHROMATICITY_SRGB = { float2(0.64f, 0.33f), float2(0.30f, 0.60f), float2(0.15f, 0.06f), WHITE_POINT_D65 };
static const Chromaticity CHROMATICITY_BT709 = CHROMATICITY_SRGB;
static const Chromaticity CHROMATICITY_BT2020 = { float2(0.708f, 0.292f), float2(0.170f, 0.797f), float2(0.131f, 0.046f), WHITE_POINT_D65 };
static const Chromaticity CHROMATICITY_ACESCG = { float2(0.713f, 0.293f), float2(0.165f, 0.830f), float2(0.128f, 0.044f), WHITE_POINT_D60 };

// These need to match vkr::ColorGamutType in core/color.h
static const uint COLOR_GAMUT_TYPE_UNKNOWN = 0;
static const uint COLOR_GAMUT_TYPE_SRGB = 1;
static const uint COLOR_GAMUT_TYPE_BT709 = 2;
static const uint COLOR_GAMUT_TYPE_BT2020 = 3;
static const uint COLOR_GAMUT_TYPE_ACESCG = 4;
static const uint COLOR_GAMUT_TYPE_COUNT = 5;

struct ColorGamut
{
    Chromaticity m_Chromaticity;
    float3 m_Primaries;
    float3x3 m_RGBToXYZ;
    float3x3 m_XYZToRGB;
    uint m_Type;
};

static const float3x3 XYZ_TO_BT709 =
{
    3.2409699419f, -1.5373831776f, -0.4986107603f,
	-0.9692436363f, 1.8759675015f, 0.0415550574f,
	 0.0556300797f, -0.2039769589f, 1.0569715142f,
};
static const float3x3 BT709_TO_XYZ =
{
    0.4123907993f, 0.3575843394f, 0.1804807884f,
	0.2126390059f, 0.7151686788f, 0.0721923154f,
	0.0193308187f, 0.1191947798f, 0.9505321522f,
};
static const ColorGamut COLOR_GAMUT_SRGB = { CHROMATICITY_SRGB, float3(0.2126390059f, 0.7151686788f, 0.0721923154f), BT709_TO_XYZ, XYZ_TO_BT709, COLOR_GAMUT_TYPE_SRGB };
static const ColorGamut COLOR_GAMUT_BT709 = { CHROMATICITY_BT709, float3(0.2126390059f, 0.7151686788f, 0.0721923154f), BT709_TO_XYZ, XYZ_TO_BT709, COLOR_GAMUT_TYPE_BT709 };

static const float3x3 XYZ_TO_BT2020 =
{
    1.7166511880f, -0.3556707838f, -0.2533662814f,
	-0.6666843518f, 1.6164812366f, 0.0157685458f,
	 0.0176398574f, -0.0427706133f, 0.9421031212f,
};
static const float3x3 BT2020_TO_XYZ =
{
    0.6369580483f, 0.1446169036f, 0.1688809752f,
	0.2627002120f, 0.6779980715f, 0.0593017165f,
	0.0000000000f, 0.0280726930f, 1.0609850577f,
};
static const ColorGamut COLOR_GAMUT_BT2020 = { CHROMATICITY_BT2020, float3(0.2627066f, 0.6779996f, 0.0592938f), BT2020_TO_XYZ, XYZ_TO_BT2020, COLOR_GAMUT_TYPE_BT2020 };

static const float3x3 XYZ_D60_TO_D65 =
{
    0.987224f, -0.00611327f, 0.0159533f,
   -0.00759836f, 1.00186f, 0.00533002f,
	0.00307257f, -0.00509595f, 1.08168f,
};
static const float3x3 XYZ_D65_TO_D60 =
{
    1.01303f, 0.00610531f, -0.014971f,
	 0.00769823f, 0.998165f, -0.00503203f,
	-0.00284131f, 0.00468516f, 0.924507f,
};

static const float3x3 AP1_TO_XYZ =
{
    0.6624541811f, 0.1340042065f, 0.1561876870f,
	 0.2722287168f, 0.6740817658f, 0.0536895174f,
	-0.0055746495f, 0.0040607335f, 1.0103391003f,
};
static const float3x3 XYZ_TO_AP1 =
{
    1.6410233797f, -0.3248032942f, -0.2364246952f,
	-0.6636628587f, 1.6153315917f, 0.0167563477f,
	 0.0117218943f, -0.0082844420f, 0.9883948585f,
};

//mul(XYZ_TO_AP1, XYZ_D65_TO_D60)
static const float3x3 XYZ_D65_TO_AP1_D60 =
{
    1.66057726f, -0.3152960112f, -0.2415096268f,
	-0.6599228016f, 1.608394097f, 0.01729866037f,
	0.009002518847f, -0.003566886195f, 0.9136441645f
};

//mul(XYZ_D60_to_D65, AP1_to_XYZ)
static const float3x3 AP1_D60_TO_XYZ_D65 =
{
    0.6522375248f, 0.128236107f, 0.1699822574f,
	0.2676717839f, 0.6743389895f, 0.05798773724f,
	-0.005381813957f, 0.001369064543f, 1.093069897f
};
static const ColorGamut COLOR_GAMUT_ACESCG = { CHROMATICITY_ACESCG, float3(0.2722287168f, 0.6740817658f, 0.0536895174f), AP1_D60_TO_XYZ_D65, XYZ_D65_TO_AP1_D60, COLOR_GAMUT_TYPE_ACESCG };

static const ColorGamut COLOR_GAMUTS[COLOR_GAMUT_TYPE_COUNT] =
{
    (ColorGamut) 0,
    COLOR_GAMUT_SRGB,
    COLOR_GAMUT_BT709,
    COLOR_GAMUT_BT2020,
    COLOR_GAMUT_ACESCG
};

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

// Bt709 transfer functions
float EncodeBt709(in float x)
{
    x = saturate(x);
    return x > 0.018f ? 1.099f * pow(x, 0.45f) - 0.099f : 4.5f * x;
}

float DecodeBt709(in float x)
{
    x = saturate(x);
    return x <= 0.081f ? (x / 4.5f) : pow((x + 0.099f) / 1.099f, 1.0f / 0.45f);
}

float3 EncodeBt709(in float3 linearRgb)
{
    return float3(EncodeBt709(linearRgb.r), EncodeBt709(linearRgb.g), EncodeBt709(linearRgb.b));
}

float3 DecodeBt709(in float3 encodedRgb)
{
    return float3(DecodeBt709(encodedRgb.r), DecodeBt709(encodedRgb.g), DecodeBt709(encodedRgb.b));
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

// These need to match vkr::DisplayEncodingType in core/color.h
static const uint DISPLAY_ENCODING_TYPE_UNKNOWN = 0;
static const uint DISPLAY_ENCODING_TYPE_LINEAR = 1;
static const uint DISPLAY_ENCODING_TYPE_SRGB = 2;
static const uint DISPLAY_ENCODING_TYPE_BT709 = 3;
static const uint DISPLAY_ENCODING_TYPE_ST2048 = 4;
static const uint DISPLAY_ENCODING_TYPE_HLG = 5;
static const uint DISPLAY_ENCODING_TYPE_COUNT = 6;

struct TransferFunction
{
    uint m_Type;
    
    float Encode(in float linearRgb)
    {
        switch (m_Type)
        {
            case DISPLAY_ENCODING_TYPE_LINEAR:
                return linearRgb;
            case DISPLAY_ENCODING_TYPE_SRGB:
                return EncodeSRGB(linearRgb);
            case DISPLAY_ENCODING_TYPE_BT709:
                return EncodeBt709(linearRgb);
            case DISPLAY_ENCODING_TYPE_ST2048:
                return EncodeSt2048(linearRgb);
            case DISPLAY_ENCODING_TYPE_HLG:
                return EncodeHLG(linearRgb);
            default:
                return linearRgb;
        }
    }
    
    float Decode(in float linearRgb)
    {
        switch (m_Type)
        {
            case DISPLAY_ENCODING_TYPE_LINEAR:
                return linearRgb;
            case DISPLAY_ENCODING_TYPE_SRGB:
                return DecodeSRGB(linearRgb);
            case DISPLAY_ENCODING_TYPE_BT709:
                return DecodeBt709(linearRgb);
            case DISPLAY_ENCODING_TYPE_ST2048:
                return DecodeSt2048(linearRgb);
            case DISPLAY_ENCODING_TYPE_HLG:
                return DecodeHLG(linearRgb);
            default:
                return linearRgb;
        }
    }
};

static const TransferFunction TRANSFER_FUNCTION_LINEAR = { DISPLAY_ENCODING_TYPE_LINEAR };
static const TransferFunction TRANSFER_FUNCTION_SRGB = { DISPLAY_ENCODING_TYPE_SRGB };
static const TransferFunction TRANSFER_FUNCTION_BT709 = { DISPLAY_ENCODING_TYPE_BT709 };
static const TransferFunction TRANSFER_FUNCTION_ST2048 = { DISPLAY_ENCODING_TYPE_ST2048 };
static const TransferFunction TRANSFER_FUNCTION_HLG = { DISPLAY_ENCODING_TYPE_HLG };
static const TransferFunction TRANSFER_FUNCTIONS[DISPLAY_ENCODING_TYPE_COUNT] =
{
    (TransferFunction)0,
    TRANSFER_FUNCTION_LINEAR,
    TRANSFER_FUNCTION_SRGB,
    TRANSFER_FUNCTION_BT709,
    TRANSFER_FUNCTION_ST2048,
    TRANSFER_FUNCTION_HLG
};

// These need to match vkr::ColorSpaceType in core/color.h
static const uint COLOR_SPACE_TYPE_UNKNOWN = 0;
static const uint COLOR_SPACE_TYPE_SRGB = 1;
static const uint COLOR_SPACE_TYPE_BT709 = 2;
static const uint COLOR_SPACE_TYPE_BT2020 = 3;
static const uint COLOR_SPACE_TYPE_ACESCG = 4;
static const uint COLOR_SPACE_TYPE_COUNT = 5;

struct ColorSpace
{
    ColorGamut m_Gamut;
    TransferFunction m_TransferFunction;
    uint m_Type;
};

static const ColorSpace COLOR_SPACE_SRGB = { COLOR_GAMUT_SRGB, TRANSFER_FUNCTION_SRGB, COLOR_SPACE_TYPE_SRGB };
static const ColorSpace COLOR_SPACE_BT709 = { COLOR_GAMUT_BT709, TRANSFER_FUNCTION_BT709, COLOR_SPACE_TYPE_BT709 };
static const ColorSpace COLOR_SPACE_BT2020 = { COLOR_GAMUT_BT2020, TRANSFER_FUNCTION_ST2048, COLOR_SPACE_TYPE_BT2020 };
static const ColorSpace COLOR_SPACE_ACESCG = { COLOR_GAMUT_ACESCG, TRANSFER_FUNCTION_LINEAR, COLOR_SPACE_TYPE_ACESCG };
static const ColorSpace COLOR_SPACE_DEFAULT = COLOR_SPACE_ACESCG;
static const ColorSpace COLOR_SPACES[COLOR_SPACE_TYPE_COUNT] =
{
    (ColorSpace)0,
    COLOR_SPACE_SRGB,
    COLOR_SPACE_BT709,
    COLOR_SPACE_BT2020,
    COLOR_SPACE_ACESCG
};

float3 EncodeColor(in float3 linearRgb, in TransferFunction transferFunction)
{
    return float3(transferFunction.Encode(linearRgb.x), transferFunction.Encode(linearRgb.y), transferFunction.Encode(linearRgb.z));
}

float3 EncodeColor(in float3 linearRgb, in ColorSpace colorSpace)
{
    return EncodeColor(linearRgb, colorSpace.m_TransferFunction);
}

float3 EncodeColor(in float3 linearRgb) // Using default transfer
{
    return EncodeColor(linearRgb, COLOR_SPACE_DEFAULT);
}

float3 DecodeColor(in float3 encodedRgb, in TransferFunction transferFunction)
{
    return float3(transferFunction.Decode(encodedRgb.x), transferFunction.Decode(encodedRgb.y), transferFunction.Decode(encodedRgb.z));
}

float3 DecodeColor(in float3 encodedRgb, in ColorSpace colorSpace)
{
    return DecodeColor(encodedRgb, colorSpace.m_TransferFunction);
}

float3 DecodeColor(in float3 encodedRgb) // Using default transfer
{
    return DecodeColor(encodedRgb, COLOR_SPACE_DEFAULT);
}

float3 TransformColor(in float3 rgb, in ColorGamut sourceGamut, in ColorGamut targetGamut)
{
    if (sourceGamut.m_Type == targetGamut.m_Type)
        return rgb;
    
    const float3x3 transform = mul(sourceGamut.m_RGBToXYZ, targetGamut.m_XYZToRGB);
    return mul(transform, rgb);
}

float3 TransformColor(in float3 rgb, in ColorSpace sourceSpace, in ColorSpace targetSpace)
{
    return TransformColor(rgb, sourceSpace.m_Gamut, targetSpace.m_Gamut);
}

float3 TransformColor(in float3 rgb, in ColorSpace targetSpace) // From default space
{
    return TransformColor(rgb, COLOR_SPACE_DEFAULT, targetSpace);
}

float GetLuminance(in float3 rgb, in ColorGamut colorGamut)
{
    return dot(rgb, colorGamut.m_Primaries);
}

float GetLuminance(in float3 rgb, in ColorSpace colorSpace)
{
    return GetLuminance(rgb, colorSpace.m_Gamut);
}

float GetLuminance(in float3 rgb) // Using default space
{
    return GetLuminance(rgb, COLOR_SPACE_DEFAULT);
}

#endif // COLOR_COMMON_HLSL