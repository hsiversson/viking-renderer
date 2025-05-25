#include "textureloader_dds.h"
#include "DirectXTex.h"

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
		outDesc.Dimension = static_cast<int>(scratch.GetMetadata().dimension); // TEXTURE2D, etc.
		outDesc.Size.x = static_cast<int>(baseImage.width);
		outDesc.Size.y = static_cast<int>(baseImage.height);
		outDesc.Size.z = 1;// static_cast<int>(baseImage.depth);
		outDesc.ArraySize = static_cast<int>(scratch.GetMetadata().arraySize);
		outDesc.Format = scratch.GetMetadata().format;
		outDesc.bUseMips = (scratch.GetMetadata().mipLevels > 1);
		outDesc.bWriteable = false; // DDS files are usually read-only

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