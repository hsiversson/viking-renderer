#pragma once
#include "render/textureloader.h"

namespace vkr::Render
{
	class TextureLoader_DDS : public TextureLoader
	{
	public:
		TextureLoader_DDS();

		bool LoadTexture(TextureDesc& outTextureDesc, TextureData& outTextureData, const std::filesystem::path& filepath) override;
	};
}