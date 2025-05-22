#include "textureloader_tga.h"

namespace vkr::Render
{

	TextureLoader_TGA::TextureLoader_TGA()
		: TextureLoader(Type::TGA)
	{

	}

	bool TextureLoader_TGA::LoadTexture(TextureDesc& outTextureDesc, TextureData& outTextureData, const std::filesystem::path& filepath)
	{
		return false;
	}
}