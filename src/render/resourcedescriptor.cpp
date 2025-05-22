#include "resourcedescriptor.h"

namespace vkr::Render
{

	ResourceDescriptor::ResourceDescriptor()
	{

	}

	ResourceDescriptor::~ResourceDescriptor()
	{

	}

	void ResourceDescriptor::Init(const D3D12_CPU_DESCRIPTOR_HANDLE& handle, uint32_t index)
	{
		m_D3DHandle = handle;
		m_DescriptorIndex = index;
	}

}