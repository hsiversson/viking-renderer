#pragma once
#include "core/types.h"
#include "render/renderstates.h"

namespace vkr::Render
{
	class Buffer;
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
	};

	class Mesh
	{
	public:
		Mesh();
		~Mesh();

		bool Init(const MeshDesc& desc);

		Ref<Render::Buffer> GetVertexBuffer() const;
		Ref<Render::Buffer> GetIndexBuffer() const;
		const Render::VertexLayout& GetVertexLayout() const;
		Render::PrimitiveTopology GetTopology() const;

	private:
		Ref<Render::Buffer> m_VertexBuffer;
		Ref<Render::Buffer> m_IndexBuffer;
		Render::VertexLayout m_VertexLayout;
		Render::PrimitiveTopology m_Topology;

		//Ref<Render::Buffer> m_BLAS;
	};
}