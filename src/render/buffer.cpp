#include "buffer.h"

namespace vkr::Render
{
	Buffer::Buffer(Device& device)
		: Resource(device)
	{

	}

	Buffer::~Buffer()
	{

	}

	bool Buffer::Init(const BufferDesc& desc, uint32_t initialDataSize, const void* initialData)
	{
		// encapsulate buffer inits in here, instead of in device.

		return false;
	}

	bool Buffer::InitWithData(uint8_t* Data, size_t size)
	{
		void* ptr;
		auto res = GetD3DResource();
		res->Map(0, nullptr, &ptr);
		memcpy(ptr, Data, size);
		res->Unmap(0, nullptr);
		return true;
	}

	const BufferDesc& Buffer::GetDesc() const
	{
		return m_Desc;
	}

}