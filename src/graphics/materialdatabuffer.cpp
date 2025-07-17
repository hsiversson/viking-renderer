#include "materialdatabuffer.h"
#include "render/device.h"
#include "render/resourcedescriptor.h"

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
		Render::Device* device = Render::GetDevice();
		if (!m_Buffer || m_Buffer->GetDesc().ByteSize() < m_Data.size())
		{
			Render::BufferDesc desc = {};
			desc.m_ElementSize = 1;
			desc.m_ElementCount = Align(m_Data.size(), 4);
			m_Buffer = device->CreateBuffer(desc, m_Data.size(), m_Data.data());

			// Always create a view over the whole buffer.
			Render::BufferViewDesc viewDesc = {};
			viewDesc.m_ElementSize = 1;
			viewDesc.m_ElementStart = 0;
			viewDesc.m_ElementCount = m_Data.size();
			viewDesc.m_Usage = Render::BUFFER_VIEW_USAGE_RAW;
			m_BufferView = device->CreateBufferView(viewDesc, m_Buffer);
		}

		if (m_RequireDataUpload)
		{
			m_Buffer->UploadData(0, m_Data.size(), m_Data.data());
			m_RequireDataUpload = false;
		}
	}

	uint32_t MaterialDataBuffer::AddData(uint32_t size, const uint8_t* data)
	{
		uint32_t offset = m_Data.size();
		m_Data.insert(m_Data.end(), data, data + size);
		m_RequireDataUpload = true;
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