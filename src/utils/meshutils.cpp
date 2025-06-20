#include "meshutils.h"

#include "render/buffer.h"
#include "render/device.h"

namespace vkr
{
	constexpr Vector3f DefaultCubeVertices[] = 
	{ 
		// Front face
		Vector3f(-0.5f, -0.5f, -0.5f), 
		Vector3f(-0.5f,  0.5f, -0.5f), 
		Vector3f( 0.5f,  0.5f, -0.5f), 
		Vector3f( 0.5f, -0.5f, -0.5f),

		// Back face
		Vector3f(-0.5f, -0.5f,  0.5f), 
		Vector3f(-0.5f,  0.5f,  0.5f), 
		Vector3f( 0.5f,  0.5f,  0.5f),
		Vector3f( 0.5f, -0.5f,  0.5f) 
	};

	constexpr uint16_t DefaultCubeIndices[] = 
	{
		// Front face
		0, 1, 2,
		0, 2, 3,

		// Back face
		4, 6, 5,
		4, 7, 6,

		// Left face
		4, 5, 1,
		4, 1, 0,

		// Right face
		3, 2, 6,
		3, 6, 7,

		// Top face
		1, 5, 6,
		1, 6, 2,

		// Bottom face
		4, 0, 3,
		4, 3, 7
	};

	Ref<Graphics::Mesh> vkr::CreateCubeMesh(Ref<Render::Device> device)
	{
		Render::BufferDesc vtxbufferdesc;
		vtxbufferdesc.bWriteOnCPU = true;
		vtxbufferdesc.m_ElementCount = sizeof(DefaultCubeVertices) / sizeof(Vector3f);
		vtxbufferdesc.m_ElementSize = sizeof(Vector3f);
		Ref<Render::Buffer> vtxbuffer = device->CreateBuffer(vtxbufferdesc);
		if (!vtxbuffer || !vtxbuffer->InitWithData((uint8_t*)(&DefaultCubeVertices), sizeof(DefaultCubeVertices)))
			return nullptr;
		Render::BufferDesc idxbufferdesc;
		idxbufferdesc.bWriteOnCPU = true;
		idxbufferdesc.m_ElementCount = sizeof(DefaultCubeIndices) / sizeof(uint16_t);
		idxbufferdesc.m_ElementSize = sizeof(uint16_t);
		Ref<Render::Buffer> idxbuffer = device->CreateBuffer(idxbufferdesc);
		if (!idxbuffer || !idxbuffer->InitWithData((uint8_t*)&DefaultCubeIndices, sizeof(DefaultCubeIndices)))
			return nullptr;
		auto mesh = MakeRef<Graphics::Mesh>();
		mesh->SetVertexBuffer(vtxbuffer);
		mesh->SetIndexBuffer(idxbuffer);
		mesh->SetTopology(Render::PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		return mesh;
	}
}
