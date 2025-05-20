#pragma once
#include "render/textureloader.h"

namespace vkr::Render
{
	class TextureLoader_PNG : public TextureLoader
	{
	public:
		TextureLoader_PNG();

		bool LoadTexture(TextureDesc& outTextureDesc, TextureData& outTextureData, const std::filesystem::path& filepath) override;
	};
}