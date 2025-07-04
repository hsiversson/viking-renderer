#include "meshutils.h"

#include "render/buffer.h"
#include "render/device.h"

namespace vkr
{
	struct CubeVertex
	{
		Vector3f position;
		Vector3f normal;
		Vector2f uv;
	};

	constexpr CubeVertex DefaultCubeVertices[24] =
	{
		//-- +Z (front) ---------------------
		{{-0.5f,-0.5f,-0.5f}, { 0.0f, 0.0f,-1.0f}, {0.0f,0.0f}},
		{{-0.5f, 0.5f,-0.5f}, { 0.0f, 0.0f,-1.0f}, {0.0f,1.0f}},
		{{ 0.5f, 0.5f,-0.5f}, { 0.0f, 0.0f,-1.0f}, {1.0f,1.0f}},
		{{ 0.5f,-0.5f,-0.5f}, { 0.0f, 0.0f,-1.0f}, {1.0f,0.0f}},

		//-- –Z (back) ----------------------
		{{ 0.5f,-0.5f, 0.5f}, { 0.0f, 0.0f, 1.0f}, {0.0f,0.0f}},
		{{ 0.5f, 0.5f, 0.5f}, { 0.0f, 0.0f, 1.0f}, {0.0f,1.0f}},
		{{-0.5f, 0.5f, 0.5f}, { 0.0f, 0.0f, 1.0f}, {1.0f,1.0f}},
		{{-0.5f,-0.5f, 0.5f}, { 0.0f, 0.0f, 1.0f}, {1.0f,0.0f}},

		//-- –X (left) ----------------------
		{{-0.5f,-0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f,0.0f}},
		{{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f,1.0f}},
		{{-0.5f, 0.5f,-0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f,1.0f}},
		{{-0.5f,-0.5f,-0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f,0.0f}},

		//-- +X (right) ---------------------
		{{ 0.5f,-0.5f,-0.5f}, { 1.0f, 0.0f, 0.0f}, {0.0f,0.0f}},
		{{ 0.5f, 0.5f,-0.5f}, { 1.0f, 0.0f, 0.0f}, {0.0f,1.0f}},
		{{ 0.5f, 0.5f, 0.5f}, { 1.0f, 0.0f, 0.0f}, {1.0f,1.0f}},
		{{ 0.5f,-0.5f, 0.5f}, { 1.0f, 0.0f, 0.0f}, {1.0f,0.0f}},

		//-- +Y (top) -----------------------
		{{-0.5f, 0.5f,-0.5f}, { 0.0f, 1.0f, 0.0f}, {0.0f,0.0f}},
		{{-0.5f, 0.5f, 0.5f}, { 0.0f, 1.0f, 0.0f}, {0.0f,1.0f}},
		{{ 0.5f, 0.5f, 0.5f}, { 0.0f, 1.0f, 0.0f}, {1.0f,1.0f}},
		{{ 0.5f, 0.5f,-0.5f}, { 0.0f, 1.0f, 0.0f}, {1.0f,0.0f}},

		//-- –Y (bottom) --------------------
		{{-0.5f,-0.5f, 0.5f}, { 0.0f,-1.0f, 0.0f}, {0.0f,0.0f}},
		{{-0.5f,-0.5f,-0.5f}, { 0.0f,-1.0f, 0.0f}, {0.0f,1.0f}},
		{{ 0.5f,-0.5f,-0.5f}, { 0.0f,-1.0f, 0.0f}, {1.0f,1.0f}},
		{{ 0.5f,-0.5f, 0.5f}, { 0.0f,-1.0f, 0.0f}, {1.0f,0.0f}}
	};

	constexpr uint16_t DefaultCubeIndices[36] =
	{
		0,1,2,    0,2,3,    // +Z
		4,5,6,    4,6,7,    // –Z
		8,9,10,   8,10,11,  // –X
		12,13,14, 12,14,15, // +X
		16,17,18, 16,18,19, // +Y
		20,21,22, 20,22,23  // –Y
	};

	Ref<Graphics::Mesh> vkr::CreateCubeMesh()
	{
		Render::BufferDesc vtxbufferdesc;
		vtxbufferdesc.m_CpuWritable = true;
		vtxbufferdesc.m_ElementCount = sizeof(DefaultCubeVertices) / sizeof(CubeVertex);
		vtxbufferdesc.m_ElementSize = sizeof(CubeVertex);
		Ref<Render::Buffer> vtxbuffer = Render::GetDevice().CreateBuffer(vtxbufferdesc, sizeof(DefaultCubeVertices), &DefaultCubeVertices);
		if (!vtxbuffer)
			return nullptr;
		Render::BufferDesc idxbufferdesc;
		idxbufferdesc.m_CpuWritable = true;
		idxbufferdesc.m_ElementCount = sizeof(DefaultCubeIndices) / sizeof(uint16_t);
		idxbufferdesc.m_ElementSize = sizeof(uint16_t);
		Ref<Render::Buffer> idxbuffer = Render::GetDevice().CreateBuffer(idxbufferdesc, sizeof(DefaultCubeIndices), &DefaultCubeIndices);
		if (!idxbuffer)
			return nullptr;
		auto mesh = MakeRef<Graphics::Mesh>();
		mesh->SetVertexBuffer(vtxbuffer);
		mesh->SetIndexBuffer(idxbuffer);
		mesh->SetTopology(Render::PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		return mesh;
	}
}
