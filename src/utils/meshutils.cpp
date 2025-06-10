#include "meshutils.h"

#include "render/buffer.h"
#include "render/device.h"

constexpr float cubeVtx[] = { -1, -1, -1, 1, -1, -1, -1, 1, -1, 1, 1, -1,
							 -1, -1,  1, 1, -1,  1, -1, 1,  1, 1, 1,  1 };

constexpr short cubeIdx[] = { 4, 6, 0, 2, 0, 6, 0, 1, 4, 5, 4, 1,
							 0, 2, 1, 3, 1, 2, 1, 3, 5, 7, 5, 3,
							 2, 6, 3, 7, 3, 6, 4, 5, 6, 7, 6, 5 };

namespace vkr
{
	Ref<Graphics::Mesh> vkr::CreateCubeMesh(Ref<Render::Device> device)
	{
		Render::BufferDesc vtxbufferdesc;
		vtxbufferdesc.bWriteOnCPU = true;
		vtxbufferdesc.m_ElementCount = sizeof(cubeVtx) / sizeof(cubeVtx[0]);
		vtxbufferdesc.m_ElementSize = sizeof(float);
		Ref<Render::Buffer> vtxbuffer = device->CreateBuffer(vtxbufferdesc);
		if (!vtxbuffer || !vtxbuffer->InitWithData((uint8_t*)(&cubeVtx), sizeof(cubeVtx)))
			return nullptr;
		Render::BufferDesc idxbufferdesc;
		idxbufferdesc.bWriteOnCPU = true;
		idxbufferdesc.m_ElementCount = sizeof(cubeIdx) / sizeof(cubeIdx[0]);
		idxbufferdesc.m_ElementSize = sizeof(short);
		Ref<Render::Buffer> idxbuffer = device->CreateBuffer(idxbufferdesc);
		if (!idxbuffer || !idxbuffer->InitWithData((uint8_t*)&cubeIdx, sizeof(cubeIdx)))
			return nullptr;
		auto mesh = MakeRef<Graphics::Mesh>();
		mesh->SetVertexBuffer(vtxbuffer);
		mesh->SetIndexBuffer(idxbuffer);
		return mesh;
	}
}
