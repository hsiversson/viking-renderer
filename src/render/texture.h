#pragma once
#include "rendercommon.h"
#include "resource.h"

namespace vkr::Render
{
	struct TextureDesc
	{
		int Dimension;
		Vector3i Size;
		int ArraySize;
		DXGI_FORMAT Format;
		bool bWriteable = false;
		bool bUseMips;
	};

	struct TextureData
	{
		std::vector<uint8_t> Data;
		uint32_t ByteSize;
	};

	class Texture : public Resource
	{
	public:
		Texture(Device& device);
		~Texture();

		TextureDesc m_TextureDesc;
	};
}