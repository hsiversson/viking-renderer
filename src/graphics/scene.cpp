#include "scene.h"
#include "material.h"
#include "mesh.h"
#include "view.h"
#include "viewmanager.h"
#include "viewrenderer.h"
#include "modelobject.h"
#include "render/device.h"

#include <functional>


namespace vkr::Graphics
{
	Scene::Scene()
		: m_HasChanges(false)
	{
		m_TraceRaysDynamicShaderLib = Render::GetDevice()->CreateShader("../../../content/shaders/tracerays_dynamic.hlsl", nullptr, Render::SHADER_STAGE_RAYTRACING);
		m_ViewManager = MakeUnique<ViewManager>(*this);
		m_ViewRenderer = MakeUnique<ViewRenderer>();
		if (!m_ViewRenderer->Init())
		{
			assert(false);
		}
	}

	Scene::~Scene()
	{

	}

	View* Scene::CreateView()
	{
		return m_ViewManager->CreateView();
	}

	void Scene::DestroyView(View* view)
	{
		m_ViewManager->DestroyView(view);
	}

	void Scene::Update()
	{
		// run updates

		// sort views based on frame structure?
		// main view goes last usually
		if (m_HasChanges)
		{
			// update tracing pipeline state

			std::vector<Render::RaytracingHitGroupDesc> materialHitGroups;
			for (size_t i = 0; i < m_SceneObjects.size(); i++)
			{
				m_SceneObjects[i]->CollectRaytracingHitGroups(materialHitGroups);
			}

			Render::PipelineStateDesc psoDesc = Render::PipelineStateDesc(Render::PIPELINE_STATE_TYPE_RAYTRACING);
			psoDesc.Raytracing.m_HitGroups = materialHitGroups.data();
			psoDesc.Raytracing.m_NumHitGroups = materialHitGroups.size();
			psoDesc.Raytracing.m_MissIdentifier = "Miss";
			psoDesc.Raytracing.m_RayGenerationIdentifier = "TraceRays";
			psoDesc.Raytracing.m_Shader = m_TraceRaysDynamicShaderLib.get();
			m_TraceRaysPipelineState = Render::GetDevice()->CreatePipelineState(psoDesc);

			m_HasChanges = false;
		}

		for (View* view : m_ViewManager->GetViews())
		{
			PrepareView(*view);
			m_ViewRenderer->RenderView(*view);
		}
	}

	void Scene::PrepareView(View& view)
	{
		// traverse all objects in Scene, add relevant ones to view.PrepareData()
		PrepareViewContext prepareViewCtx(view);
		ViewRenderData& prepareData = view.GetPrepareData();

		prepareData.m_TraceRaysPipelineState = m_TraceRaysPipelineState;

		for (size_t i = 0; i < m_SceneObjects.size(); i++)
		{
			m_SceneObjects[i]->CollectRenderObjects(prepareData);
		}

		std::sort(prepareData.m_VisibleMeshes.begin(), prepareData.m_VisibleMeshes.end());

		auto DepthPSOSelector = [](RenderObject* obj)->Ref<Render::PipelineState>
		{
			return obj->m_Material->GetMaterial()->GetDepthPipelineState(obj->m_Mesh->GetVertexLayout());
		};
		auto DefaultPSOSelector = [](RenderObject* obj)->Ref<Render::PipelineState>
		{
			return obj->m_Material->GetMaterial()->GetDefaultPipelineState(obj->m_Mesh->GetVertexLayout());
		};

		//Pass batch collection
		auto CollectBatchesForPass = [&prepareData](MeshPassData& PassData,std::function<Ref<Render::PipelineState>(RenderObject*)> PSOSelector) {
			if (!prepareData.m_VisibleMeshes.size())
				return;
			RenderObject* referenceObject = &prepareData.m_VisibleMeshes[0];
			RenderBatch currentBatch;
			currentBatch.m_Mesh = referenceObject->m_Mesh;
			currentBatch.m_PSO = PSOSelector(referenceObject);
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

					currentBatch.m_StartOffset = prepareData.m_InstanceDataOffsetBuffer.size();
					currentBatch.m_Count = 1;
					referenceObject = &(*it);
				}
				prepareData.m_InstanceDataOffsetBuffer.push_back(it->m_InstanceDataIndex);
			}
			//Add last batch
			PassData.m_InstanceBatches.push_back(currentBatch);
		};
		
		CollectBatchesForPass(prepareData.m_DepthPassData, DepthPSOSelector);
		if (false) //(!useRaytracing)
		{
			CollectBatchesForPass(prepareData.m_ForwardPassData, DefaultPSOSelector);
		}
	}
}