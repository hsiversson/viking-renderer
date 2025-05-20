#include "texture.h"

namespace vkr::Render
{
	Texture::Texture(Device& device)
		: DeviceObject(device)
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