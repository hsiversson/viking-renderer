#pragma once
#include "rendercommon.h"
#include "resource.h"

namespace vkr::Render
{
	struct BufferDesc
	{
		Format m_Format;
		uint32_t m_ElementSize;

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

		void UploadData(uint32_t byteSize, const void* data);
		void DownloadData();

		bool InitWithData(uint8_t* Data, size_t size);

		const BufferDesc& GetDesc() const;

	private:
		BufferDesc m_Desc;
	};

	struct TempBuffer
	{
		uint64_t m_Offset;
		Buffer* m_Buffer;
	};
}