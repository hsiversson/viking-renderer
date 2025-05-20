#include "buffer.h"

namespace vkr::Render
{
	Buffer::Buffer(Device& device)
		: DeviceObject(device)
	{

	}

	Buffer::~Buffer()
	{

	}

	bool Buffer::Init(ID3D12Resource* resource)
	{
		m_Resource = resource;
		return true;
	}

}