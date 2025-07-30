#pragma once
#include "core/types.h"
#include "render/renderstates.h"

namespace vkr::Render
{
	class Buffer;
	class BufferView;
}

namespace vkr::Graphics
{
	struct MeshDesc
	{
		Render::PrimitiveTopology m_Topology;
		Render::VertexLayout m_VertexLayout;
		uint32_t m_NumVertices;
		std::unordered_map<Render::VertexAttribute::Type, std::vector<uint8_t>> m_VertexData; // TODO: multiple sets per attribute?

		Render::Format m_IndexFormat;
		uint32_t m_NumIndices;
		std::vector<uint8_t> m_IndexData;

		bool m_IncludeInRaytracing;
	};

	class Mesh
	{
	public:
		Mesh();
		~Mesh();

		bool Init(const MeshDesc& desc);

		const Ref<Render::Buffer>& GetVertexBuffer() const;
		const Ref<Render::Buffer>& GetIndexBuffer() const;
		const Ref<Render::Buffer>& GetBLAS() const;

		const Render::VertexLayout& GetVertexLayout() const;
		Render::PrimitiveTopology GetTopology() const;

		const Ref<Render::BufferView>& GetRaytraceVBView() const { return m_RaytraceVBView; }
		const Ref<Render::BufferView>& GetRaytraceIBView() const { return m_RaytraceIBView; }

	private:
		Ref<Render::Buffer> m_VertexBuffer;
		Ref<Render::Buffer> m_IndexBuffer;
		Ref<Render::BufferView> m_RaytraceVBView;
		Ref<Render::BufferView> m_RaytraceIBView;
		Render::VertexLayout m_VertexLayout;
		Render::PrimitiveTopology m_Topology;

		Ref<Render::Buffer> m_BLAS;
	};
}