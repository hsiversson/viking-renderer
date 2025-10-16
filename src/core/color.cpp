#include "color.h"

namespace vkr
{
	const Chromaticity Chromaticity::XYZ	= { Vector2f(1.000f, 0.000f), Vector2f(0.000f, 1.000f), Vector2f(0.000f, 0.000f), WhitePoint::E };
	const Chromaticity Chromaticity::XYZD65 = { Vector2f(1.000f, 0.000f), Vector2f(0.000f, 1.000f), Vector2f(0.000f, 0.000f), WhitePoint::D65 };
	const Chromaticity Chromaticity::sRGB	= { Vector2f(0.640f, 0.330f), Vector2f(0.300f, 0.600f), Vector2f(0.150f, 0.060f), WhitePoint::D65 };
	const Chromaticity Chromaticity::Bt709	= Chromaticity::sRGB;
	const Chromaticity Chromaticity::Bt2020	= { Vector2f(0.708f, 0.292f), Vector2f(0.170f, 0.797f), Vector2f(0.131f, 0.046f), WhitePoint::D65 };
	const Chromaticity Chromaticity::ACEScg	= { Vector2f(0.713f, 0.293f), Vector2f(0.165f, 0.830f), Vector2f(0.128f, 0.044f), WhitePoint::D60 };

	static constexpr Mat33 XYZ_TO_BT709 =
	{
		 3.240969941904523f, -1.537383177570094f, -0.498610760293003f,
		-0.969243636280880f,  1.875967501507721f,  0.041555057407176f,
		 0.055630079696994f, -0.203976958888977f,  1.056971514242879f,
	};
	static constexpr Mat33 BT709_TO_XYZ =
	{
		0.412390799265959f, 0.357584339383878f, 0.180480788401834f,
		0.212639005871510f, 0.715168678767756f, 0.072192315360734f,
		0.019330818715592f, 0.119194779794626f, 0.950532152249661f,
	};
	const ColorGamut ColorGamut::sRGB =
	{
		Chromaticity::sRGB,
		Vector3f(0.2126390059f, 0.7151686788f, 0.0721923154f),
		BT709_TO_XYZ,
		XYZ_TO_BT709,
		COLOR_GAMUT_TYPE_SRGB
	};
	const ColorGamut ColorGamut::Bt709 =
	{
		Chromaticity::Bt709,
		Vector3f(0.2126390059f, 0.7151686788f, 0.0721923154f),
		BT709_TO_XYZ,
		XYZ_TO_BT709,
		COLOR_GAMUT_TYPE_BT709
	};

	static constexpr Mat33 XYZ_TO_BT2020 =
	{
		 1.716651187971268f, -0.355670783776392f, -0.253366281373660f,
		-0.666684351832489f,  1.616481236634939f,  0.0157685458139111f,
		 0.017639857445311f, -0.042770613257809f,  0.942103121235474f
	};
	static constexpr Mat33 BT2020_TO_XYZ =
	{
		0.6369580483012914f, 0.14461690358620832f,  0.1688809751641721f,
		0.2627002120112671f, 0.6779980715188708f,   0.05930171646986196f,
		0.000000000000000f,  0.028072693049087428f, 1.060985057710791f
	};
	const ColorGamut ColorGamut::Bt2020 =
	{
		Chromaticity::Bt2020,
		Vector3f(0.2627066f, 0.6779996f, 0.0592938f),
		BT2020_TO_XYZ,
		XYZ_TO_BT2020,
		COLOR_GAMUT_TYPE_BT2020
	};

	static constexpr Mat33 XYZ_D60_TO_D65 =
	{
		 0.9872662656994802f, -0.0060928634285899f, 0.0159463602354658f,
		-0.0075983600000000f,  1.0018606938107803f, 0.0053310309342503f,
		 0.0030721800000000f, -0.0050951500000000f, 1.0816825569602853f,
	};
	static constexpr Mat33 XYZ_D65_TO_D60 =
	{
		 1.0130349182778702f, 0.0061053141993125f, -0.0149703475586551f,
		 0.0076982294268071f, 0.9981654561081410f, -0.0050320323388590f,
		-0.0028413170880970f, 0.0046852581218020f,  0.9245070982797407f,
	};
	static constexpr Mat33 AP1_TO_XYZ =
	{
		 0.6624541811085053f,   0.13400420645643313f, 0.1561876870049078f,
		 0.27222871678091454f,  0.6740817658111481f,  0.05368951740793705f,
		-0.005574649490394297f, 0.0040607335289828f,  1.010339100312997f,
	};
	static constexpr Mat33 XYZ_TO_AP1 =
	{
		 1.6410233796943252f,   -0.32480329418479f,    -0.23642469523761225f,
		-0.6636628587229829f,    1.6153315916573379f,   0.01675634768553013f,
		 0.011721894328375376f, -0.008284442012526075f, 0.9883948585390231f,
	};
	const ColorGamut ColorGamut::ACEScg =
	{
		Chromaticity::ACEScg,
		Vector3f(0.2722287168f, 0.6740817658f, 0.0536895174f),
		AP1_TO_XYZ * XYZ_D60_TO_D65,
		XYZ_D65_TO_D60 * XYZ_TO_AP1,
		COLOR_GAMUT_TYPE_ACESCG
	};

	// sRGB transfer functions
	static float locEncodeSRGB(float x)
	{
		x = Saturate(x);
		return (x <= 0.0031308f) ? 12.92f * x : 1.055f * pow(x, 1.0f / 2.4f) - 0.055f;
	}
	static float locDecodeSRGB(float x)
	{
		x = Saturate(x);
		return (x <= 0.04045f) ? x / 12.92f : pow((x + 0.055f) / 1.055f, 2.4f);
	}

	// Bt709 transfer functions
	static float locEncodeBt709(float x)
	{ 
		return x > 0.018f ? 1.099f * std::pow(x, 0.45f) - 0.099f : 4.5f * x; 
	}
	static float locDecodeBt709(float x)
	{ 
		return x <= 0.081f ? (x / 4.5f) : std::pow((x + 0.099f) / 1.099f, 1.0f / 0.45f);
	}

	// SMPTE ST 2084 (PQ) transfer functions
	// Luminance (nits) range: [0, 10000]
	static constexpr float St2048_m1 = (2610.0f / 16384.0f);
	static constexpr float St2048_m2 = (2523.0f / 32.0f);
	static constexpr float St2048_c1 = (3424.0f / 4096.0f);
	static constexpr float St2048_c2 = (2413.0f / 128.0f);
	static constexpr float St2048_c3 = (2392.0f / 128.0f);
	static constexpr float St2048_ep = 10000.0f; // Encoding peak brightness

	static float locEncodeSt2048(float L)
	{
		L = std::max(0.0f, L);
		float Lm1 = std::pow(L, St2048_m1);
		float num = St2048_c1 + St2048_c2 * Lm1;
		float den = 1.0f + St2048_c3 * Lm1;
		return std::pow(num / std::max(den, 1e-9f), St2048_m2);
	}
	static float locDecodeSt2048(float N)
	{
		N = Saturate(N);
		float Np = std::pow(N, 1.0f / St2048_m2);
		float num = std::max(Np - St2048_c1, 0.0f);
		float den = St2048_c2 - St2048_c3 * Np;
		return std::pow(num / std::max(den, 1e-9f), 1.0f / St2048_m1); // nits, up to 10000
	}

	// HLG (ITU-R BT.2100) transfer functions
	static constexpr float HLG_a = 0.17883277f;
	static constexpr float HLG_b = 1.0f - 4.0f * HLG_a;
	static const float HLG_c = 0.5f - HLG_a * std::log(4.0f * HLG_a);

	static float locEncodeHLG(float L)
	{
		L = std::max(L, 0.0f);
		return (L <= 1.0f / 12.0) ? std::sqrt(3.0f * L) : HLG_a * std::log(12.0f * L - HLG_b) + HLG_c;
	}
	static float locDecodeHLG(float V)
	{
		V = Saturate(V);
		return (V <= 0.5f) ? (V * V) / 3.0f : (std::exp((V - HLG_c) / HLG_a) + HLG_b) / 12.0f;
	}

	const TransferFunction TransferFunction::Linear = { [](float x) { return x; }, [](float x) { return x; }, DISPLAY_ENCODING_TYPE_LINEAR };
	const TransferFunction TransferFunction::sRGB	= { locEncodeSRGB, locDecodeSRGB, DISPLAY_ENCODING_TYPE_SRGB };
	const TransferFunction TransferFunction::Bt709	= { locEncodeBt709, locDecodeBt709, DISPLAY_ENCODING_TYPE_BT709 };
	const TransferFunction TransferFunction::St2048 = { locEncodeSt2048, locDecodeSt2048, DISPLAY_ENCODING_TYPE_ST2048 };
	const TransferFunction TransferFunction::HLG	= { locEncodeHLG, locDecodeHLG, DISPLAY_ENCODING_TYPE_HLG };

	const ColorSpace ColorSpace::sRGB = { "sRGB", ColorGamut::sRGB, TransferFunction::sRGB, COLOR_SPACE_TYPE_SRGB };
	//const ColorSpace ColorSpace::scRGB = { "scRGB", ColorGamut::scRGB, TransferFunction::scRGB };
	const ColorSpace ColorSpace::Bt709 = { "Bt709", ColorGamut::Bt709, TransferFunction::Bt709, COLOR_SPACE_TYPE_BT709 };
	const ColorSpace ColorSpace::Bt2020 = { "Bt2020", ColorGamut::Bt2020, TransferFunction::St2048, COLOR_SPACE_TYPE_BT2020 };
	const ColorSpace ColorSpace::ACEScg = { "ACEScg", ColorGamut::ACEScg, TransferFunction::Linear, COLOR_SPACE_TYPE_ACESCG };

	Vector3f EncodeColor(const Vector3f& linearRgb, const TransferFunction& transferFunction)
	{
		return Vector3f(transferFunction.Encode(linearRgb.x), transferFunction.Encode(linearRgb.y), transferFunction.Encode(linearRgb.z));
	}

	Vector3f EncodeColor(const Vector3f& linearRgb, const ColorSpace& colorSpace)
	{
		return EncodeColor(linearRgb, colorSpace.m_TransferFunction);
	}

	Vector3f EncodeColor(const Vector3f& linearRgb)
	{
		return EncodeColor(linearRgb, ColorSpace::DefaultSpace());
	}

	Vector3f DecodeColor(const Vector3f& encodedRgb, const TransferFunction& transferFunction)
	{
		return Vector3f(transferFunction.Decode(encodedRgb.x), transferFunction.Decode(encodedRgb.y), transferFunction.Decode(encodedRgb.z));
	}

	Vector3f DecodeColor(const Vector3f& encodedRgb, const ColorSpace& colorSpace)
	{
		return DecodeColor(encodedRgb, colorSpace.m_TransferFunction);
	}

	Vector3f DecodeColor(const Vector3f& encodedRgb)
	{
		return DecodeColor(encodedRgb, ColorSpace::DefaultSpace());
	}

	Vector3f TransformColor(const Vector3f& rgb, const ColorGamut& sourceGamut, const ColorGamut& targetGamut)
	{
		const Mat33 transform = sourceGamut.m_RGBToXYZ * targetGamut.m_XYZToRGB;
		return transform * rgb;
	}

	Vector3f TransformColor(const Vector3f& rgb, const ColorSpace& sourceSpace, const ColorSpace& targetSpace)
	{
		return TransformColor(rgb, sourceSpace.m_Gamut, targetSpace.m_Gamut);
	}

	Vector3f TransformColor(const Vector3f& rgb, const ColorSpace& targetSpace)
	{
		return TransformColor(rgb, ColorSpace::DefaultSpace(), targetSpace);
	}

	float GetLuminance(const Vector3f& rgb, const ColorGamut& colorGamut)
	{
		return Dot(rgb, colorGamut.m_Primaries);
	}

	float GetLuminance(const Vector3f& rgb, const ColorSpace& colorSpace)
	{
		return GetLuminance(rgb, colorSpace.m_Gamut);
	}

	float GetLuminance(const Vector3f& rgb)
	{
		return GetLuminance(rgb, ColorSpace::DefaultSpace());
	}

	Vector2f GetChromaticityXY(const Vector3f& rgb, const ColorGamut& sourceGamut)
	{
		Vector3f cieXYZ = sourceGamut.m_RGBToXYZ * rgb;

		float divisor = std::max((cieXYZ.x + cieXYZ.y + cieXYZ.z), 1e-8f);

		Vector2f coordinate;
		coordinate.x = cieXYZ.x / divisor;
		coordinate.y = cieXYZ.y / divisor;
		return coordinate;
	}

	Vector2f GetChromaticityXY(const Vector3f& rgb, const ColorSpace& sourceSpace)
	{
		return GetChromaticityXY(rgb, sourceSpace.m_Gamut);
	}

	Vector2f GetChromaticityXY(const Vector3f& rgb)
	{
		return GetChromaticityXY(rgb, ColorSpace::DefaultSpace());
	}
}