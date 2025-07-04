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
		DirectX::DDS_FLAGS flags = DirectX::DDS_FLAGS_FORCE_DX10_EXT;
		DirectX::TexMetadata metaData;
		DirectX::ScratchImage scratch;

		HRESULT hr = DirectX::LoadFromDDSFile(filepath.c_str(), flags, &metaData, scratch);
		if (FAILED(hr))
			return false;

		const DirectX::Image* images = scratch.GetImages();
		const size_t numImages = scratch.GetImageCount();

		for (size_t i = 0; i < numImages; ++i)
		{
			const DirectX::Image& image = images[i];
			if (i == 0)
			{
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

				outDesc.m_Size.x = static_cast<uint32_t>(image.width);
				outDesc.m_Size.y = static_cast<uint32_t>(image.height);
				outDesc.m_Size.z = static_cast<uint32_t>(metaData.depth);
				outDesc.m_ArraySize = static_cast<uint16_t>(metaData.arraySize);
				outDesc.m_Format = D3DConvertFormat(metaData.format);
				outDesc.m_MipLevels = metaData.mipLevels;
				outDesc.m_Writable = false; // DDS files are usually read-only
			}

			TextureData::Subresource mipData;
			mipData.m_Data.resize(image.slicePitch);
			mipData.m_RowPitch = image.rowPitch;
			mipData.m_SlicePitch = image.slicePitch;
			memcpy(mipData.m_Data.data(), image.pixels, image.slicePitch);

			outData.m_Subresources.push_back(mipData);
		}

		return true;
	}
}