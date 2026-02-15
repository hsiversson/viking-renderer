#pragma once

#include "Core/memory.h"
#include "Graphics/SceneObject.h"

using namespace vkr;

namespace vkr::Render
{
	class Buffer;
	class PipelineState;
	class Shader;
	class Texture;
};

namespace vkr::Graphics
{
	class MaterialInstance;
	class View;

	class Terrain
	{
	public:
		Ref<Render::Texture> m_Heightmap; //Initial single heightmap for all scene
		Ref<MaterialInstance> m_TerrainMaterial;
		//Double-buffered data for terrain updatig every frame
		Ref<Render::Buffer> m_VertexBuffers[2];
		Ref<Render::Buffer> m_IndexBuffers[2];
		Ref<Render::Buffer> m_BLAS[2];
		Ref<Render::BufferView> m_VertexBufferViews[2];
		Ref<Render::BufferView> m_IndexBufferViews[2];
		Render::VertexLayout m_VertexLayout;
	};

	class TerrainSceneObject : public PrimitiveSceneObject
	{
	public:
		TerrainSceneObject(Ref<Terrain> terrain);
		virtual void CollectRenderObjects(ViewRenderData& renderdata, const std::unordered_map<Material*, uint32_t>& hitGroupLibrary) override;
		virtual void GatherMaterials(std::unordered_set<Material*>& outMaterials) override;

	private:
		Ref<Terrain> m_Terrain;
		uint32_t m_CurrentMesh; //Index of the current mesh to be used for rendering
	};

	class TerrainRenderer
	{
	public:
		TerrainRenderer() = default;
		~TerrainRenderer() = default;

		bool Init();

		void GenerateClipmapMesh(View* view);

	private:
		Ref<Render::Shader> m_ClipmapMeshCS;
		Ref<Render::PipelineState> m_ClipmapMeshPSO;
	};
};