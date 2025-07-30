#include "mesh.h"
#include "render/buffer.h"
#include "render/device.h"

namespace vkr::Graphics
{
	Mesh::Mesh()
		: m_Topology(Render::PRIMITIVE_TOPOLOGY_UNDEFINED)
	{
	}

	Mesh::~Mesh()
	{

	}

	bool Mesh::Init(const MeshDesc& desc)
	{
		Render::Device* device = Render::GetDevice();

		// Make interleaved vertex buffer
		// TODO: Do we want to interleave optionally?
		std::vector<uint8_t> vertexBufferData;
		for (uint32_t i = 0; i < desc.m_NumVertices; ++i)
		{
			for (const auto& attr : desc.m_VertexLayout.GetAttributes())
			{
				const std::vector<uint8_t>& attrData = desc.m_VertexData.at(attr.m_Type);
				const uint32_t attrSize = GetFormatBytesPerPixel(attr.m_Format);
				const size_t offset = i * attrSize;
				const uint8_t* attrStart = attrData.data() + offset;
				vertexBufferData.insert(vertexBufferData.end(), attrStart, attrStart + attrSize);
			}
		}

		Render::BufferDesc vertexBufferDesc;
		vertexBufferDesc.m_CpuWritable = true;
		vertexBufferDesc.m_ElementSize = desc.m_VertexLayout.GetStride();
		vertexBufferDesc.m_ElementCount = desc.m_NumVertices;
		m_VertexBuffer = device->CreateBuffer(vertexBufferDesc, vertexBufferData.size(), vertexBufferData.data());
		if (!m_VertexBuffer)
			return false;

		Render::BufferDesc indexBufferDesc;
		indexBufferDesc.m_CpuWritable = true;
		indexBufferDesc.m_ElementSize = GetFormatBytesPerPixel(desc.m_IndexFormat);
		indexBufferDesc.m_ElementCount = desc.m_NumIndices;
		indexBufferDesc.m_Format = desc.m_IndexFormat;
		m_IndexBuffer = device->CreateBuffer(indexBufferDesc, desc.m_IndexData.size(), desc.m_IndexData.data());
		if (!m_IndexBuffer)
			return false;

		//These views will be used for raytracing
		Render::BufferViewDesc srvVBDesc;
		srvVBDesc.m_Usage = Render::BUFFER_VIEW_USAGE_RAW;
		srvVBDesc.m_ElementCount = desc.m_NumVertices * desc.m_VertexLayout.GetStride();
		srvVBDesc.m_ElementSize = 1;
		srvVBDesc.m_ElementStart = 0;
		srvVBDesc.m_Format = Render::FORMAT_UNKNOWN;
		m_RaytraceVBView = device->CreateBufferView(srvVBDesc, m_VertexBuffer);

		Render::BufferViewDesc srvIBDesc;
		srvIBDesc.m_Usage = Render::BUFFER_VIEW_USAGE_RAW;
		srvIBDesc.m_ElementCount = desc.m_NumIndices * GetFormatBytesPerPixel(desc.m_IndexFormat);
		srvIBDesc.m_ElementSize = 1;
		srvIBDesc.m_ElementStart = 0;
		m_RaytraceIBView = device->CreateBufferView(srvIBDesc, m_IndexBuffer);

		m_Topology = desc.m_Topology;
		m_VertexLayout = desc.m_VertexLayout;

		if (true/*desc.m_IncludeInRaytracing*/)
		{
			Render::RaytracingGeometryDesc rtGeometryDesc = {};
			rtGeometryDesc.m_VertexBuffer = m_VertexBuffer;
			rtGeometryDesc.m_IndexBuffer = m_IndexBuffer;
			m_BLAS = device->CreateBLAS(1, &rtGeometryDesc);
		}

		return true;
	}

	const Ref<Render::Buffer>& Mesh::GetVertexBuffer() const
	{
		return m_VertexBuffer;
	}

	const Ref<Render::Buffer>& Mesh::GetIndexBuffer() const
	{
		return m_IndexBuffer;
	}

	const Ref<Render::Buffer>& Mesh::GetBLAS() const
	{
		return m_BLAS;
	}

	const Render::VertexLayout& Mesh::GetVertexLayout() const
	{
		return m_VertexLayout;
	}

	Render::PrimitiveTopology Mesh::GetTopology() const
	{
		return m_Topology;
	}

}
