#pragma once
#include "render/textureloader.h"

namespace vkr::Render
{
	class TextureLoader_TGA : public TextureLoader
	{
	public:
		TextureLoader_TGA();

		bool LoadTexture(TextureDesc& outTextureDesc, TextureData& outTextureData, const std::filesystem::path& filepath) override;
	};
}