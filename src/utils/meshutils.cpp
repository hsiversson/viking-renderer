#include "meshutils.h"

#include "render/buffer.h"
#include "render/device.h"

namespace vkr
{
	constexpr Vector3f DefaultCubePositions[24] =
	{
		// +Z (front)
		{-0.5f,-0.5f,-0.5f},
		{-0.5f, 0.5f,-0.5f},
		{ 0.5f, 0.5f,-0.5f},
		{ 0.5f,-0.5f,-0.5f},

		// –Z (back)
		{ 0.5f,-0.5f, 0.5f},
		{ 0.5f, 0.5f, 0.5f},
		{-0.5f, 0.5f, 0.5f},
		{-0.5f,-0.5f, 0.5f},

		// –X (left)
		{-0.5f,-0.5f, 0.5f},
		{-0.5f, 0.5f, 0.5f},
		{-0.5f, 0.5f,-0.5f},
		{-0.5f,-0.5f,-0.5f},

		// +X (right)
		{ 0.5f,-0.5f,-0.5f},
		{ 0.5f, 0.5f,-0.5f},
		{ 0.5f, 0.5f, 0.5f},
		{ 0.5f,-0.5f, 0.5f},

		// +Y (top)
		{-0.5f, 0.5f,-0.5f},
		{-0.5f, 0.5f, 0.5f},
		{ 0.5f, 0.5f, 0.5f},
		{ 0.5f, 0.5f,-0.5f},

		// –Y (bottom)
		{-0.5f,-0.5f, 0.5f},
		{-0.5f,-0.5f,-0.5f},
		{ 0.5f,-0.5f,-0.5f},
		{ 0.5f,-0.5f, 0.5f}
	};

	constexpr Vector3f DefaultCubeNormals[24] =
	{
		// +Z (front)
		{ 0.0f, 0.0f,-1.0f},
		{ 0.0f, 0.0f,-1.0f},
		{ 0.0f, 0.0f,-1.0f},
		{ 0.0f, 0.0f,-1.0f},

		// –Z (back)
		{ 0.0f, 0.0f, 1.0f},
		{ 0.0f, 0.0f, 1.0f},
		{ 0.0f, 0.0f, 1.0f},
		{ 0.0f, 0.0f, 1.0f},

		// –X (left)
		{-1.0f, 0.0f, 0.0f},
		{-1.0f, 0.0f, 0.0f},
		{-1.0f, 0.0f, 0.0f},
		{-1.0f, 0.0f, 0.0f},

		// +X (right)
		{ 1.0f, 0.0f, 0.0f},
		{ 1.0f, 0.0f, 0.0f},
		{ 1.0f, 0.0f, 0.0f},
		{ 1.0f, 0.0f, 0.0f},

		// +Y (top)
		{ 0.0f, 1.0f, 0.0f},
		{ 0.0f, 1.0f, 0.0f},
		{ 0.0f, 1.0f, 0.0f},
		{ 0.0f, 1.0f, 0.0f},

		// –Y (bottom)
		{ 0.0f,-1.0f, 0.0f},
		{ 0.0f,-1.0f, 0.0f},
		{ 0.0f,-1.0f, 0.0f},
		{ 0.0f,-1.0f, 0.0f}
	};

	constexpr Vector2f DefaultCubeUvs[24] =
	{
		// +Z (front)
		{0.0f,0.0f},
		{0.0f,1.0f},
		{1.0f,1.0f},
		{1.0f,0.0f},

		// –Z (back)
		{0.0f,0.0f},
		{0.0f,1.0f},
		{1.0f,1.0f},
		{1.0f,0.0f},

		// –X (left)
		{0.0f,0.0f},
		{0.0f,1.0f},
		{1.0f,1.0f},
		{1.0f,0.0f},

		// +X (right) --
		{0.0f,0.0f},
		{0.0f,1.0f},
		{1.0f,1.0f},
		{1.0f,0.0f},

		// +Y (top)
		{0.0f,0.0f},
		{0.0f,1.0f},
		{1.0f,1.0f},
		{1.0f,0.0f},

		// –Y (bottom)
		{0.0f,0.0f},
		{0.0f,1.0f},
		{1.0f,1.0f},
		{1.0f,0.0f}
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
		Graphics::MeshDesc meshDesc = {};
		meshDesc.m_NumVertices = 24;
		meshDesc.m_NumIndices = 36;
		meshDesc.m_Topology = Render::PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		Render::VertexAttribute attribute;
		attribute.m_Type = Render::VertexAttribute::TYPE_POSITION;
		attribute.m_Index = 0;
		attribute.m_BufferSlot = 0;
		attribute.m_Format = Render::FORMAT_RGB32_FLOAT;
		meshDesc.m_VertexLayout.m_Attributes.insert(attribute);
		meshDesc.m_VertexData[Render::VertexAttribute::TYPE_POSITION].resize(sizeof(DefaultCubePositions));
		memcpy(meshDesc.m_VertexData[Render::VertexAttribute::TYPE_POSITION].data(), &DefaultCubePositions, sizeof(DefaultCubePositions));

		attribute.m_Type = Render::VertexAttribute::TYPE_NORMAL;
		attribute.m_Format = Render::FORMAT_RGB32_FLOAT;
		meshDesc.m_VertexLayout.m_Attributes.insert(attribute);
		meshDesc.m_VertexData[Render::VertexAttribute::TYPE_NORMAL].resize(sizeof(DefaultCubeNormals));
		memcpy(meshDesc.m_VertexData[Render::VertexAttribute::TYPE_NORMAL].data(), &DefaultCubeNormals, sizeof(DefaultCubeNormals));

		attribute.m_Type = Render::VertexAttribute::TYPE_UV;
		attribute.m_Format = Render::FORMAT_RG32_FLOAT;
		meshDesc.m_VertexLayout.m_Attributes.insert(attribute);
		meshDesc.m_VertexData[Render::VertexAttribute::TYPE_UV].resize(sizeof(DefaultCubeUvs));
		memcpy(meshDesc.m_VertexData[Render::VertexAttribute::TYPE_UV].data(), &DefaultCubeUvs, sizeof(DefaultCubeUvs));

		meshDesc.m_IndexFormat = Render::FORMAT_R16_UINT;
		meshDesc.m_IndexData.resize(sizeof(DefaultCubeIndices));
		memcpy(meshDesc.m_IndexData.data(), &DefaultCubeIndices, sizeof(DefaultCubeIndices));

		Ref<Graphics::Mesh> mesh = MakeRef<Graphics::Mesh>();
		if (!mesh->Init(meshDesc))
			return nullptr;

		return mesh;
	}
}
