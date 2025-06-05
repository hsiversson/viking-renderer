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
		Buffer(Device& device);
		~Buffer();

		bool Init(const BufferDesc& desc, uint32_t initialDataSize = 0, const void* initialData = nullptr);

		void UploadData();
		void DownloadData();

		bool InitWithData(uint8_t* Data, size_t size);
	};
}