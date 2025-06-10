#include "texture.h"

namespace vkr::Render
{
	Texture::Texture()
	{

	}

	Texture::~Texture()
	{

	}

	bool Texture::Init(const TextureDesc& desc, const TextureData* initialData)
	{
		// encapsulate texture inits in here, instead of in device.

		return false;
	}
}