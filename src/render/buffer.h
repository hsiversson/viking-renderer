#pragma once
#include "rendercommon.h"
#include "resource.h"

namespace vkr::Render
{
	struct BufferDesc
	{
		Format m_Format;
		uint32_t m_ElementSize;
		uint32_t m_ElementCount;
		bool m_Writable = false;
		bool m_CpuWritable = false;

		inline uint32_t ByteSize() const { return m_ElementCount * m_ElementSize; }
	};

	class Buffer : public Resource
	{
	public:
		Buffer();
		~Buffer();

		bool Init(const BufferDesc& desc, uint32_t initialDataSize = 0, const void* initialData = nullptr);

		void UploadData(uint64_t offset, uint32_t byteSize, const void* data);
		void DownloadData();

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