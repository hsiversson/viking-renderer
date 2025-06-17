#pragma once
#include "rendercommon.h"
#include "resource.h"

namespace vkr::Render
{
	struct TextureDesc
	{
		int Dimension = 2;
		Vector3i Size = {1,1,0};
		int ArraySize = 1;
		DXGI_FORMAT Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		bool bWriteable = false;
		bool bUseMips = true;
		bool bDepthStencil = false;
	};

	struct TextureData
	{
		std::vector<uint8_t> Data;
		uint32_t ByteSize;
	};

	class Texture : public Resource
	{
	public:
		Texture();
		~Texture();

		bool Init(const TextureDesc& desc, const TextureData* initialData = nullptr);

		TextureDesc m_TextureDesc;
	};
}