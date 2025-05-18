#include "texture.h"

namespace vkr::Render
{
	Texture::Texture()
	{

	}

	Texture::~Texture()
	{

	}

	bool Texture::Init(ID3D12Resource* resource)
	{
		m_Resource = resource;
		return true;
	}

}