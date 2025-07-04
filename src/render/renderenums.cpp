#include "renderenums.h"

namespace vkr::Render
{
	uint32_t GetFormatBitsPerPixel(Format format)
	{
		switch (format)
		{
		default:
		case FORMAT_UNKNOWN:
			return 0;

		case FORMAT_RGBA32_FLOAT:
		case FORMAT_RGBA32_UINT:
		case FORMAT_RGBA32_SINT:
			return 128;

		case FORMAT_RGB32_FLOAT:
		case FORMAT_RGB32_UINT:
		case FORMAT_RGB32_SINT:
			return 96;

		case FORMAT_RGBA16_FLOAT:
		case FORMAT_RGBA16_UNORM:
		case FORMAT_RGBA16_SNORM:
		case FORMAT_RGBA16_UINT:
		case FORMAT_RGBA16_SINT:
		case FORMAT_RG32_FLOAT:
		case FORMAT_RG32_UINT:
		case FORMAT_RG32_SINT:
			return 64;

		case FORMAT_RGBA8_UNORM:
		case FORMAT_RGBA8_UNORM_SRGB:
		case FORMAT_RGBA8_SNORM:
		case FORMAT_RGBA8_UINT:
		case FORMAT_RGBA8_SINT:
		case FORMAT_RG16_FLOAT:
		case FORMAT_RG16_UNORM:
		case FORMAT_RG16_SNORM:
		case FORMAT_RG16_UINT:
		case FORMAT_RG16_SINT:
		case FORMAT_D32_FLOAT:
		case FORMAT_R32_FLOAT:
		case FORMAT_R32_UINT:
		case FORMAT_R32_SINT:
		case FORMAT_RGB10A2_UNORM:
		case FORMAT_RGB10A2_UINT:
		case FORMAT_RG11B10_FLOAT:
		case FORMAT_RGB9E5_SE:
			return 32;

		case FORMAT_R16_FLOAT:
		case FORMAT_D16_UNORM:
		case FORMAT_R16_UNORM:
		case FORMAT_R16_SNORM:
		case FORMAT_R16_UINT:
		case FORMAT_R16_SINT:
		case FORMAT_RG8_UNORM:
		case FORMAT_RG8_SNORM:
		case FORMAT_RG8_SINT:
		case FORMAT_RG8_UINT:
			return 16;

		case FORMAT_R8_UNORM:
		case FORMAT_R8_SNORM:
		case FORMAT_R8_SINT:
		case FORMAT_R8_UINT:
		case FORMAT_BC2:
		case FORMAT_BC3:
		case FORMAT_BC5_UNORM:
		case FORMAT_BC5_SNORM:
		case FORMAT_BC6_UFLOAT:
		case FORMAT_BC6_SFLOAT:
		case FORMAT_BC7:
		case FORMAT_BC7_SRGB:
			return 8;

		case FORMAT_BC1:
		case FORMAT_BC4_UNORM:
		case FORMAT_BC4_SNORM:
			return 4;
		};
	}

	uint32_t GetFormatBytesPerPixel(Format format)
	{
		return GetFormatBitsPerPixel(format) / 8;
	}

	uint32_t GetFormatBlockSize(Format format)
	{
		switch (format)
		{
		case FORMAT_BC1:
		case FORMAT_BC2:
		case FORMAT_BC3:
		case FORMAT_BC4_UNORM:
		case FORMAT_BC4_SNORM:
		case FORMAT_BC5_UNORM:
		case FORMAT_BC5_SNORM:
		case FORMAT_BC6_UFLOAT:
		case FORMAT_BC6_SFLOAT:
		case FORMAT_BC7:
		case FORMAT_BC7_SRGB:
			return 4;
		default:
			return 1;
		}
	}
}

