#include "textureloader_dds.h"
#include "DirectXTex.h"
#include "d3dconvert.h"

namespace vkr::Render
{
	TextureLoader_DDS::TextureLoader_DDS()
		: TextureLoader(Type::DDS)
	{

	}
	
	bool TextureLoader_DDS::LoadTexture(TextureDesc& outDesc, TextureData& outData, const std::filesystem::path& filepath)
	{
		DirectX::ScratchImage scratch = {};
		HRESULT hr = DirectX::LoadFromDDSFileEx(filepath.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, nullptr, scratch);
		if (FAILED(hr))
			return false;

		const DirectX::Image* img = scratch.GetImages();
		size_t imgCount = scratch.GetImageCount();

		if (!img || imgCount == 0)
			return false;

		// We'll just read the first image slice here (most common case)
		const DirectX::Image& baseImage = img[0];

		// Fill outDesc
		switch (scratch.GetMetadata().dimension)
		{
		case DirectX::TEX_DIMENSION_TEXTURE1D:
			outDesc.m_Dimension = ResourceDimension::Texture1D;
			break;
		case DirectX::TEX_DIMENSION_TEXTURE2D:
			outDesc.m_Dimension = ResourceDimension::Texture2D;
			break;
		case DirectX::TEX_DIMENSION_TEXTURE3D:
			outDesc.m_Dimension = ResourceDimension::Texture3D;
			break;
		default:
			assert(false);
			return false;
		}

		outDesc.m_Size.x = static_cast<uint32_t>(baseImage.width);
		outDesc.m_Size.y = static_cast<uint32_t>(baseImage.height);
		outDesc.m_Size.z = 1;
		outDesc.m_ArraySize = static_cast<uint16_t>(scratch.GetMetadata().arraySize);
		outDesc.m_Format = D3DConvertFormat(scratch.GetMetadata().format);
		outDesc.m_CalculateMips = (scratch.GetMetadata().mipLevels > 1);
		outDesc.m_Writeable = false; // DDS files are usually read-only

		// Copy pixel data into outData
		// Total bytes = sum of all image slices * bytes per slice
		size_t totalBytes = scratch.GetPixelsSize();
		outData.Data.resize(totalBytes);

		// Copy all pixel data (including all mip levels and array slices)
		memcpy(outData.Data.data(), scratch.GetPixels(), totalBytes);
		outData.ByteSize = static_cast<uint32_t>(totalBytes);

		return true;
	}
}