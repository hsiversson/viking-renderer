#pragma once
#include "utils/types.h"

namespace vkr::Render
{
	class Buffer;
}

namespace vkr::Graphics
{
	class Mesh
	{
	public:
		Mesh();
		~Mesh();

		bool Init();

	private:
		Ref<Render::Buffer> m_VertexBuffer;
		Ref<Render::Buffer> m_IndexBuffer;

		//Ref<Render::Buffer> m_BLAS;
	};
}