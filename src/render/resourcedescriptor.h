#pragma once

#include "rendercommon.h"

namespace vkr::Render
{
	enum class ResourceDescriptorType : unsigned int
	{
		UAV = 0,
		SRV = 1,
		NumTypes
	};

	struct BufferDescriptorDesc
	{
		unsigned int First = 0;
		unsigned int Last = 0;
		unsigned int ElementSize = 0;

	};

	struct TextureDescriptorDesc
	{
		unsigned int Mip = 0; //Use -1 to reference all mips when applicable
	};

	struct ResourceDescriptorDesc
	{
		ResourceDescriptorType Type = ResourceDescriptorType::NumTypes;
		union
		{
			BufferDescriptorDesc BufferDesc;
			TextureDescriptorDesc TextureDesc;
		};
	};

	class ResourceDescriptor
	{
	public:
		ResourceDescriptor();
		~ResourceDescriptor();

		void Init(const D3D12_CPU_DESCRIPTOR_HANDLE& handle, uint32_t index);
		uint32_t GetIndex() const { return m_DescriptorIndex; }
		D3D12_CPU_DESCRIPTOR_HANDLE& GetHandle() { return m_D3DHandle; }
	private:
		D3D12_CPU_DESCRIPTOR_HANDLE m_D3DHandle;
		uint32_t m_DescriptorIndex;
	};
}