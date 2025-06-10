#pragma once
#include "utils/types.h"

namespace vkr::Render
{
	class Buffer;
}

using namespace vkr::Render;

namespace vkr::Graphics
{
	class Mesh
	{
	public:
		Mesh();
		~Mesh();

		bool Init();
		void SetVertexBuffer(Ref<Buffer> vtxbuffer) { m_VertexBuffer = vtxbuffer; }
		Ref<Buffer> GetVertexBuffer() const { return m_VertexBuffer; }
		void SetIndexBuffer(Ref<Buffer> idxbuffer) { m_VertexBuffer = idxbuffer; }
		Ref<Buffer> GetIndexBuffer() const { return m_IndexBuffer; }

	private:
		Ref<Render::Buffer> m_VertexBuffer;
		Ref<Render::Buffer> m_IndexBuffer;

		//Ref<Render::Buffer> m_BLAS;
	};
}