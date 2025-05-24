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
		void SetIndexBuffer(Ref<Buffer> idxbuffer) { m_VertexBuffer = idxbuffer; }

	private:
		Ref<Render::Buffer> m_VertexBuffer;
		Ref<Render::Buffer> m_IndexBuffer;

		//Ref<Render::Buffer> m_BLAS;
	};
}