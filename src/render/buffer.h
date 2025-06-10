#pragma once
#include "rendercommon.h"
#include "Resource.h"

namespace vkr::Render
{
	struct BufferDesc
	{
		unsigned int ElementCount;
		unsigned int ElementSize;
		bool bWriteOnCPU = true;
		bool bWriteOnGPU = false;
	};

	class Buffer : public Resource
	{
	public:
		Buffer(Device& device);
		~Buffer();

		bool InitWithData(uint8_t* Data, size_t size);

		BufferDesc m_BufferDesc;
	};
}