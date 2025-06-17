#pragma once
#include "utils/types.h"
#include "render/renderstates.h"

namespace vkr::Render
{
	class Buffer;
}

namespace vkr::Graphics
{
	struct MeshDesc
	{
		Render::VertexLayout m_VertexLayout;
		std::unordered_map<Render::VertexAttribute::Type, std::vector<uint8_t>> m_VertexData;
		std::vector<uint32_t> m_Indices;
	};

	class Mesh
	{
	public:
		Mesh();
		~Mesh();

		bool Init(const MeshDesc& desc);
		void SetVertexBuffer(Ref<Render::Buffer> vtxbuffer) { m_VertexBuffer = vtxbuffer; }
		void SetIndexBuffer(Ref<Render::Buffer> idxbuffer) { m_IndexBuffer = idxbuffer; }
		void SetTopology(Render::PrimitiveTopology topologyType) { m_Topology = topologyType; }
		Ref<Render::Buffer> GetVertexBuffer() const { return m_VertexBuffer; }
		Ref<Render::Buffer>  GetIndexBuffer() const { return m_IndexBuffer; }
		Render::PrimitiveTopology GetTopology() const { return m_Topology; }

	private:
		bool InitVertexBuffer(const MeshDesc& desc);
		bool InitIndexBuffer(const MeshDesc& desc);

		Ref<Render::Buffer> m_VertexBuffer;
		Ref<Render::Buffer> m_IndexBuffer;
		Render::PrimitiveTopology m_Topology;

		//Ref<Render::Buffer> m_BLAS;
	};
}