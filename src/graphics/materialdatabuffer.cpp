#include "materialdatabuffer.h"

namespace vkr::Graphics
{
	MaterialDataBuffer::MaterialDataBuffer()
	{

	}

	MaterialDataBuffer::~MaterialDataBuffer()
	{

	}

	void MaterialDataBuffer::Clear()
	{
		m_Data.clear();
	}

	void MaterialDataBuffer::PrepareBuffer()
	{
	}

	uint32_t MaterialDataBuffer::AddData(uint32_t size, const uint8_t* data)
	{
		uint32_t offset = m_Data.size();
		m_Data.insert(m_Data.end(), data, data + size);
		return offset;
	}

	const std::vector<uint8_t>& MaterialDataBuffer::GetData() const
	{
		return m_Data;
	}

	Render::Buffer* MaterialDataBuffer::GetBuffer() const
	{
		return m_Buffer.get();
	}

}