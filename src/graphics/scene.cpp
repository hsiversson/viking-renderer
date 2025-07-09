#include "scene.h"
#include "material.h"
#include "mesh.h"
#include "view.h"

#include <functional>


namespace vkr::Graphics
{
	Scene::Scene()
	{

	}

	Scene::~Scene()
	{

	}

	Ref<View> Scene::CreateView()
	{
		Ref<View> view = MakeRef<View>();
		m_Views.push_back(view);
		return view;
	}

	void Scene::DestroyView(const Ref<View>& view)
	{
		const auto offset = std::find(m_Views.begin(), m_Views.end(), view);
		m_Views[offset - m_Views.begin()] = m_Views.back();
		m_Views.pop_back();
	}

	void Scene::Update()
	{
		// run updates

		// sort views based on frame structure?
		// main view goes last usually
	}

	void Scene::PrepareView(View& view)
	{
		// traverse all objects in Scene, add relevant ones to view.PrepareData()
		PrepareViewContext prepareViewCtx(view);
		ViewRenderData& prepareData = view.GetPrepareData();

		for (size_t i = 0; i < m_SceneObjects.size(); i++)
		{
			m_SceneObjects[i]->CollectRenderObjects(prepareData);
		}

		std::sort(prepareData.m_VisibleMeshes.begin(), prepareData.m_VisibleMeshes.end());

		auto DepthPSOSelector = [](RenderObject* obj)->Ref<Render::PipelineState>
		{
				return obj->m_Material->GetDepthPipelineState(obj->m_Mesh->GetVertexLayout());
		};
		auto DefaultPSOSelector = [](RenderObject* obj)->Ref<Render::PipelineState>
		{
			return obj->m_Material->GetDefaultPipelineState(obj->m_Mesh->GetVertexLayout());
		};

		//Pass batch collection
		auto CollectBatchesForPass = [&prepareData](MeshPassData& PassData,std::function<Ref<Render::PipelineState>(RenderObject*)> PSOSelector) {
			if (!prepareData.m_VisibleMeshes.size())
				return;
			RenderObject* referenceObject = &prepareData.m_VisibleMeshes[0];
			RenderBatch currentBatch;
			currentBatch.m_Mesh = referenceObject->m_Mesh;
			currentBatch.m_PSO = PSOSelector(referenceObject);
			currentBatch.m_TextureIndex = referenceObject->m_Material->GetTexture(0) ? referenceObject->m_Material->GetTexture(0)->GetIndex() : 0;
			currentBatch.m_StartOffset = prepareData.m_InstanceDataOffsetBuffer.size();

			for (auto it = prepareData.m_VisibleMeshes.begin(); it != prepareData.m_VisibleMeshes.end(); it++)
			{
				//For now material IDs well leave them (were not filling material parameters or using them anyway YET)
				if (referenceObject->m_Mesh == it->m_Mesh && PSOSelector(referenceObject) == PSOSelector(&(*it)))
				{
					currentBatch.m_Count++;
				}
				else
				{
					PassData.m_InstanceBatches.push_back(currentBatch);
					currentBatch.m_Mesh = it->m_Mesh;
					currentBatch.m_PSO = PSOSelector(&(*it));
					currentBatch.m_TextureIndex = it->m_Material->GetTexture(0) ? it->m_Material->GetTexture(0)->GetIndex() : 0;
					currentBatch.m_StartOffset = prepareData.m_InstanceDataOffsetBuffer.size();
					currentBatch.m_Count = 1;
				}
				prepareData.m_InstanceDataOffsetBuffer.push_back(it->m_InstanceDataIndex);
			}
			//Add last batch
			PassData.m_InstanceBatches.push_back(currentBatch);
		};
		
		CollectBatchesForPass(prepareData.m_DepthPassData, DepthPSOSelector);
		CollectBatchesForPass(prepareData.m_ForwardPassData, DefaultPSOSelector);
		
		Ref<Render::Buffer> rtTLAS = Render::GetDevice()->CreateTLAS(prepareData.m_RaytracingInstances.size(), prepareData.m_RaytracingInstances.data());

		Render::BufferViewDesc rtTLASDesc = {};
		rtTLASDesc.m_IsRaytracingAccelerationStructure = true;
		prepareData.m_RaytracingTLAS = Render::GetDevice()->CreateBufferView(rtTLASDesc, rtTLAS);
	}
}