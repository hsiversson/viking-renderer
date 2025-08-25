#pragma once
#include "common.h"

namespace vkr
{
	struct WhitePoint
	{
		static constexpr Vector2f D65 = Vector2f(0.31271f, 0.32902f);
		static constexpr Vector2f D60 = Vector2f(0.32168f, 0.33767f);
		static constexpr Vector2f E = Vector2f(1.0f/3.0f, 1.0f/3.0f);
	};

	struct Chromaticity
	{
		const Vector2f m_Red;
		const Vector2f m_Green;
		const Vector2f m_Blue;
		const Vector2f m_White;

		static const Chromaticity XYZ;
		static const Chromaticity XYZD65;
		static const Chromaticity sRGB;
		static const Chromaticity Bt709;
		static const Chromaticity Bt2020;
		static const Chromaticity ACEScg;
	};

	struct ColorGamut
	{
		const Chromaticity& m_Chromaticity;
		const Vector3f m_Primaries;
		const Mat33 m_RGBToXYZ;
		const Mat33 m_XYZToRGB;

		static const ColorGamut sRGB;
		static const ColorGamut Bt709;
		static const ColorGamut Bt2020;
		static const ColorGamut ACEScg;
	};

	struct TransferFunction
	{
		float (*Encode)(float linearRgb);
		float (*Decode)(float encodedRgb);

		static const TransferFunction Linear;
		static const TransferFunction sRGB;
		static const TransferFunction Bt709;
		static const TransferFunction St2048;
		static const TransferFunction HLG;
	};

	struct ColorSpace
	{
		const char* m_DisplayName;
		const ColorGamut m_Gamut;
		const TransferFunction m_TransferFunction;

		static const ColorSpace sRGB;
		//static const ColorSpace scRGB;
		static const ColorSpace Bt709;
		static const ColorSpace Bt2020;
		static const ColorSpace ACEScg;

		static const ColorSpace& DefaultSpace() { return ACEScg; }
	};

	Vector3f EncodeColor(const Vector3f& linearRgb, const TransferFunction& transferFunction);
	Vector3f EncodeColor(const Vector3f& linearRgb, const ColorSpace& colorSpace);
	Vector3f EncodeColor(const Vector3f& linearRgb); // Using default transfer

	Vector3f DecodeColor(const Vector3f& encodedRgb, const TransferFunction& transferFunction);
	Vector3f DecodeColor(const Vector3f& encodedRgb, const ColorSpace& colorSpace);
	Vector3f DecodeColor(const Vector3f& encodedRgb); // Using default transfer

	Vector3f TransformColor(const Vector3f& rgb, const ColorGamut& sourceGamut, const ColorGamut& targetGamut);
	Vector3f TransformColor(const Vector3f& rgb, const ColorSpace& sourceSpace, const ColorSpace& targetSpace);
	Vector3f TransformColor(const Vector3f& rgb, const ColorSpace& targetSpace); // From default space

	float GetLuminance(const Vector3f& rgb, const ColorGamut& colorGamut);
	float GetLuminance(const Vector3f& rgb, const ColorSpace& colorSpace);
	float GetLuminance(const Vector3f& rgb); // Using default space

	Vector2f GetChromaticityXY(const Vector3f& rgb, const ColorGamut& sourceGamut);
	Vector2f GetChromaticityXY(const Vector3f& rgb, const ColorSpace& sourceSpace);
	Vector2f GetChromaticityXY(const Vector3f& rgb); // From default space
}