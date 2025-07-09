#pragma once
#include "light.h"
#include "render/buffer.h"
#include "render/device.h"

namespace vkr::Render
{
	class PipelineState;
	class BufferView;
}

namespace vkr::Graphics
{
	class Mesh;
	class Material;
	class PipelineState;

	//For now lets do fixed instance data
	struct MaterialData
	{
		uint32_t m_textureDescriptorIndex0;
		uint32_t m_textureDescriptorIndex1;
		uint32_t m_textureDescriptorIndex2;
		uint32_t m_textureDescriptorIndex3;
		uint32_t m_textureDescriptorIndex4;
		uint32_t m_textureDescriptorIndex5;
		uint32_t m_textureDescriptorIndex6;
		uint32_t m_textureDescriptorIndex7;
	};

	struct InstanceData
	{
		Mat44 m_Transform;
		uint32_t m_MaterialID;
	};

	struct RenderObject
	{
		Mesh* m_Mesh;
		Material* m_Material;
		uint32_t m_InstanceDataIndex;
		float m_DistanceToCamera;

		// State-sort operator material->mesh->distance
		bool operator<(const RenderObject& other) const
		{
			if (m_Material != other.m_Material)
				return m_Material < other.m_Material;

			if (m_Mesh != other.m_Mesh)
				return m_Mesh < other.m_Mesh;

			return m_DistanceToCamera < other.m_DistanceToCamera;
		}
	};

	struct RenderBatch
	{
		Mesh* m_Mesh = nullptr;
		Ref<Render::PipelineState> m_PSO = nullptr;
		size_t m_StartOffset = 0;
		size_t m_Count = 0;
	};

	struct MeshPassData
	{
		std::vector<RenderBatch> m_InstanceBatches;
	};

	struct ViewRenderData
	{
		void Clear();

		Ref<Render::BufferView> m_RaytracingTLAS;

		std::vector<Render::RaytracingInstanceDesc> m_RaytracingInstances;
		std::vector<RenderObject> m_VisibleMeshes;
		std::vector<Light> m_VisibleLights;
		MeshPassData m_DepthPassData;
		MeshPassData m_ForwardPassData;
		std::vector<uint8_t> m_InstanceData; //We might want to take this outside of the view prepare , and do it at scene update time
		Ref<Render::BufferView> m_InstanceDataBufferView;
		//Contains the set of indices into the instance data buffer to retrieve the instance data for every instance to render in the batches
		std::vector<uint32_t> m_InstanceDataOffsetBuffer;
		Ref<Render::BufferView> m_InstanceDataOffsetBufferView;
		Render::TempBuffer m_PerSceneConstantBuffer;
		
		uint32_t m_TotalInstanceCount = 0;
		std::vector<MaterialData> m_MaterialDataBuffer;
	};
}