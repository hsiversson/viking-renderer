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
		BufferDesc vtxbufferdesc;
		vtxbufferdesc.bWriteOnCPU = true;
		vtxbufferdesc.Size = sizeof(cubeVtx);
		Ref<Buffer> vtxbuffer = device->CreateBuffer(vtxbufferdesc);
		if (!vtxbuffer || !vtxbuffer->InitWithData((uint8_t*)(&cubeVtx), vtxbufferdesc.Size))
			return nullptr;
		BufferDesc idxbufferdesc;
		idxbufferdesc.bWriteOnCPU = true;
		idxbufferdesc.Size = sizeof(cubeIdx);
		Ref<Buffer> idxbuffer = device->CreateBuffer(idxbufferdesc);
		if (!idxbuffer || !idxbuffer->InitWithData((uint8_t*)&cubeIdx, idxbufferdesc.Size))
			return nullptr;
		auto mesh = MakeRef<Graphics::Mesh>();
		mesh->SetVertexBuffer(vtxbuffer);
		mesh->SetIndexBuffer(idxbuffer);
	}
}
