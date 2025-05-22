#include "textureloader_dds.h"
#include <fstream>

namespace vkr::Render
{
	struct DDS_PIXELFORMAT 
	{
		uint32_t size;
		uint32_t flags;
		uint32_t fourCC;
		uint32_t rgbBitCount;
		uint32_t rBitMask;
		uint32_t gBitMask;
		uint32_t bBitMask;
		uint32_t aBitMask;
	};

	struct DDS_HEADER 
	{
		uint32_t magic;
		uint32_t size;
		uint32_t flags;
		uint32_t height;
		uint32_t width;
		uint32_t pitchOrLinearSize;
		uint32_t depth;
		uint32_t mipMapCount;
		uint32_t reserved1[11];
		DDS_PIXELFORMAT ddspf;
		uint32_t caps;
		uint32_t caps2;
		uint32_t caps3;
		uint32_t caps4;
		uint32_t reserved2;
	};

	struct DDS_HEADER_DXT10 
	{
		DXGI_FORMAT dxgiFormat;
		uint32_t resourceDimension;
		uint32_t miscFlag;
		uint32_t arraySize;
		uint32_t miscFlags2;
	};

	DXGI_FORMAT FourCCToDXGI(uint32_t fourCC) 
	{
		switch (fourCC) 
		{
		case '1TXD': return DXGI_FORMAT_BC1_UNORM; // "DXT1"
		case '3TXD': return DXGI_FORMAT_BC2_UNORM; // "DXT3"
		case '5TXD': return DXGI_FORMAT_BC3_UNORM; // "DXT5"
		case 'U4CB': return DXGI_FORMAT_BC4_UNORM;
		case '21CB': return DXGI_FORMAT_BC5_UNORM;
		case '71CB': return DXGI_FORMAT_BC7_UNORM;
		default: return DXGI_FORMAT_UNKNOWN;
		}
	}

	size_t CalculateDDSDataSize(uint32_t width, uint32_t height, uint32_t mipLevels, DXGI_FORMAT format) 
	{
		// Only rough size estimate for BC formats
		size_t totalSize = 0;
		for (uint32_t i = 0; i < mipLevels; ++i) 
		{
			uint32_t w = std::max(1u, width >> i);
			uint32_t h = std::max(1u, height >> i);

			uint32_t blockWidth = (w + 3) / 4;
			uint32_t blockHeight = (h + 3) / 4;
			uint32_t blockSize = 0;

			switch (format) 
			{
			case DXGI_FORMAT_BC1_UNORM:
			case DXGI_FORMAT_BC4_UNORM:
				blockSize = 8;
				break;
			case DXGI_FORMAT_BC2_UNORM:
			case DXGI_FORMAT_BC3_UNORM:
			case DXGI_FORMAT_BC5_UNORM:
			case DXGI_FORMAT_BC7_UNORM:
				blockSize = 16;
				break;
			default:
				return 0; // unsupported format
			}

			totalSize += blockWidth * blockHeight * blockSize;
		}
		return totalSize;
	}

	TextureLoader_DDS::TextureLoader_DDS()
		: TextureLoader(Type::DDS)
	{

	}

	bool TextureLoader_DDS::LoadTexture(TextureDesc& outDesc, TextureData& outData, const std::filesystem::path& filepath)
	{
		std::ifstream file(filepath, std::ios::binary);
		if (!file.is_open())
			return false;

		DDS_HEADER header{};
		file.read(reinterpret_cast<char*>(&header), sizeof(DDS_HEADER));

		if (header.magic != 0x20534444 || header.size != 124 || header.ddspf.size != 32)
			return false;

		DXGI_FORMAT format = FourCCToDXGI(header.ddspf.fourCC);
		if (format == DXGI_FORMAT_UNKNOWN)
			return false;

		uint32_t width = header.width;
		uint32_t height = header.height;
		uint32_t mipCount = header.mipMapCount ? header.mipMapCount : 1;

		outDesc.Dimension = 2;
		outDesc.Size = { static_cast<int32_t>(width), static_cast<int32_t>(height), 1 };
		outDesc.ArraySize = 1;
		outDesc.Format = format;
		outDesc.bUseMips = mipCount > 1;

		size_t dataSize = CalculateDDSDataSize(width, height, mipCount, format);
		if (dataSize == 0)
			return false;

		outData.Data.resize(dataSize);
		file.read(reinterpret_cast<char*>(outData.Data.data()), dataSize);
		outData.ByteSize = static_cast<uint32_t>(dataSize);

		return true;
	}
}