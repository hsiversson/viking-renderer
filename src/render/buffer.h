#pragma once
#include "rendercommon.h"
#include "Resource.h"

namespace vkr::Render
{
	struct BufferDesc
	{
		unsigned int Size;
		bool bWriteOnCPU = true;
		bool bWriteOnGPU = false;
	};

	class Buffer : public Resource
	{
	public:
		Buffer();
		~Buffer();
	};
}