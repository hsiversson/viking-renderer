#include "textureloader_png.h"

namespace vkr::Render
{

	TextureLoader_PNG::TextureLoader_PNG()
		: TextureLoader(Type::PNG)
	{

	}

	bool TextureLoader_PNG::LoadTexture(TextureDesc& outTextureDesc, TextureData& outTextureData, const std::filesystem::path& filepath)
	{
		return false;
	}
}